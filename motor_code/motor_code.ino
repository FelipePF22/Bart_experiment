#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "time.h"

// ---- Config WiFi ----
const char* ssid = "user";
const char* password = "password";

// ---- Status WiFi ----
const int ledPin = 2;

// ---- Pinos dos sensores ----
#define Sensor1 26
#define Sensor2 25
#define Sensor3 17
#define Sensor4 16

// ---- Variáveis globais ----
volatile unsigned long periodo1 = 0;
volatile unsigned long periodo2 = 0;
volatile unsigned long periodo3 = 0;
volatile unsigned long periodo4 = 0;

unsigned long ultimo_temp1 = 0;
unsigned long ultimo_temp2 = 0;
unsigned long ultimo_temp3 = 0;
unsigned long ultimo_temp4 = 0;

// ---- Web Server ----
WebServer server(80);

// ---- Interrupções ----
void IRAM_ATTR aciona_Interrupcao1() {
  unsigned long agora = millis();
  periodo1 = agora - ultimo_temp1;
  ultimo_temp1 = agora;
}
void IRAM_ATTR aciona_Interrupcao2() {
  unsigned long agora = millis();
  periodo2 = agora - ultimo_temp2;
  ultimo_temp2 = agora;
}
void IRAM_ATTR aciona_Interrupcao3() {
  unsigned long agora = millis();
  periodo3 = agora - ultimo_temp3;
  ultimo_temp3 = agora;
}
void IRAM_ATTR aciona_Interrupcao4() {
  unsigned long agora = millis();
  periodo4 = agora - ultimo_temp4;
  ultimo_temp4 = agora;
}

// ---- Pega hora formatada ----
String getHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00:00"; // fallback
  }
  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

// ---- Rotas do servidor ----
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Monitoramento de Sensores</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body { background-color: #f8f9fa; }
    .card-title { color: #fd0d0d; }
    .card-text { font-size: 2.5rem; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container mt-5">
    <h1 class="text-center text-success mb-5">Bart experiment (sensor monitoring) </h1>
    <div class="row row-cols-1 row-cols-md-2 g-4">
      <div class="col">
        <div class="card text-center shadow-sm">
          <div class="card-body">
            <h5 class="card-title">Sensor 1</h5>
            <p class="card-text text-dark" id="sensor1-val">Aguardando...</p>
          </div>
        </div>
      </div>
      <div class="col">
        <div class="card text-center shadow-sm">
          <div class="card-body">
            <h5 class="card-title">Sensor 2</h5>
            <p class="card-text text-dark" id="sensor2-val">Aguardando...</p>
          </div>
        </div>
      </div>
      <div class="col">
        <div class="card text-center shadow-sm">
          <div class="card-body">
            <h5 class="card-title">Sensor 3</h5>
            <p class="card-text text-dark" id="sensor3-val">Aguardando...</p>
          </div>
        </div>
      </div>
      <div class="col">
        <div class="card text-center shadow-sm">
          <div class="card-body">
            <h5 class="card-title">Sensor 4</h5>
            <p class="card-text text-dark" id="sensor4-val">Aguardando...</p>
          </div>
        </div>
      </div>
    </div>
    
    <div class="text-center mt-5">
      <a href="/download" class="btn btn-success btn-lg shadow-sm">Baixar Log</a>
      <a href="/json" class="btn btn-secondary btn-lg shadow-sm">Ver JSON</a>
    </div>
    
    <p class="text-muted text-center mt-4">Última atualização: <span id="hora"></span></p>
  </div>

  <script>
    function getDados() {
      fetch('/json')
        .then(response => response.json())
        .then(data => {
          document.getElementById('sensor1-val').innerText = data.sensor1 + ' ms';
          document.getElementById('sensor2-val').innerText = data.sensor2 + ' ms';
          document.getElementById('sensor3-val').innerText = data.sensor3 + ' ms';
          document.getElementById('sensor4-val').innerText = data.sensor4 + ' ms';
          document.getElementById('hora').innerText = data.hora;
        })
        .catch(error => console.error('Erro ao buscar dados:', error));
    }
    
    setInterval(getDados, 1000); 
    getDados();
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleJSON() {
  String json = "{";
  json += "\"hora\":\"" + getHora() + "\",";
  json += "\"sensor1\":" + String(periodo1) + ",";
  json += "\"sensor2\":" + String(periodo2) + ",";
  json += "\"sensor3\":" + String(periodo3) + ",";
  json += "\"sensor4\":" + String(periodo4);
  json += "}";
  server.send(200, "application/json", json);
}

void handleDownload() {
  File file = SPIFFS.open("/sensores.txt", FILE_READ);
  if (!file) {
    server.send(500, "text/plain", "Erro ao abrir arquivo");
    return;
  }
  server.streamFile(file, "text/plain");
  file.close();
}

void setup() {
  Serial.begin(115200);

  // Configura pinos
  pinMode(Sensor1, INPUT);
  pinMode(Sensor2, INPUT);
  pinMode(Sensor3, INPUT);
  pinMode(Sensor4, INPUT);

  // Configura o led de status
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  

  // Interrupções
  attachInterrupt(digitalPinToInterrupt(Sensor1), aciona_Interrupcao1, FALLING);
  attachInterrupt(digitalPinToInterrupt(Sensor2), aciona_Interrupcao2, FALLING);
  attachInterrupt(digitalPinToInterrupt(Sensor3), aciona_Interrupcao3, FALLING);
  attachInterrupt(digitalPinToInterrupt(Sensor4), aciona_Interrupcao4, FALLING);

  // Monta SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS!");
    return;
  }

  // --- Adicionado para resetar o log ---
  if (SPIFFS.exists("/sensores.txt")) {
    SPIFFS.remove("/sensores.txt");
    Serial.println("Arquivo de log antigo removido!");
  }
  // ------------------------------------

  // Conexão WiFi
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, HIGH);

  // Config NTP
  configTime(-3 * 3600, 0, "pool.ntp.org"); // UTC-3 (Brasil)
  Serial.println("Sincronizando hora via NTP...");

  // Se arquivo não existe → cria com cabeçalho (agora sempre será criado após o remove)
  if (!SPIFFS.exists("/sensores.txt")) {
    File file = SPIFFS.open("/sensores.txt", FILE_WRITE);
    if (file) {
      file.println("Hora;Sensor1(ms);Sensor2(ms);Sensor3(ms);Sensor4(ms)");
      file.close();
    }
  }

  // Rotas
  server.on("/", handleRoot);
  server.on("/json", handleJSON);
  server.on("/download", handleDownload);
  server.begin();
}

void loop() {
  // Lógica de reconexão de Wi-Fi no loop()
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, LOW);
    Serial.println("Desconectado! Tentando reconectar...");
    WiFi.reconnect();
    // Você pode remover o delay(5000); aqui se quiser,
    // já que o ESP32 vai tentar reconectar por conta própria no próximo loop.
    // Manter o delay pode ajudar a evitar "flashes" rápidos do LED.
    delay(5000); 
  } else {
    digitalWrite(ledPin, HIGH);
    server.handleClient();

    unsigned long agora = millis();

    // ---- Timeout de 1s sem evento → zera ----
    if (agora - ultimo_temp1 > 1000) periodo1 = 0;
    if (agora - ultimo_temp2 > 1000) periodo2 = 0;
    if (agora - ultimo_temp3 > 1000) periodo3 = 0;
    if (agora - ultimo_temp4 > 1000) periodo4 = 0;

    // ---- Grava log a cada 1s ----
    static unsigned long ultimoLog = 0;
    if (agora - ultimoLog > 1000) {
      ultimoLog = agora;
      
      File file = SPIFFS.open("/sensores.txt", FILE_APPEND);
      if (file) {
        file.printf("%s;%lu;%lu;%lu;%lu\n",
                    getHora().c_str(),
                    periodo1, periodo2, periodo3, periodo4);
        file.close();
      }
    }
  }
}

