/*
  ESP32 Low-Latency Web Car Controller (Built-in Standard WebServer)
  - Creates Wi-Fi Access Point: "ESP32_Car_WiFi"
  - Password: "password123"
  - URL: http://192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

// Motor Pin Definitions
const int ENA = 14;
const int IN1 = 27;
const int IN2 = 26;
const int IN3 = 25;
const int IN4 = 33;
const int ENB = 32;

int currentSpeed = 180; // Speed range: 0-255

// Wi-Fi Access Point Credentials
const char* ssid = "ESP32_Car_WiFi";
const char* password = "password123";

WebServer server(80);

void setSpeed(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void moveForward()  { setSpeed(currentSpeed); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void moveBackward() { setSpeed(currentSpeed); digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void turnLeft()     { setSpeed(currentSpeed); digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void turnRight()    { setSpeed(currentSpeed); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void stopCar()      { setSpeed(0);            digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }

// Webpage UI (Optimized Touch Handling + Keep-Alive)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>ESP32 RC Controller</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #121212; color: #ffffff; margin: 0; padding: 20px; user-select: none; -webkit-user-select: none; }
    h2 { margin-bottom: 15px; color: #00e676; }
    .grid { display: grid; grid-template-columns: repeat(3, 85px); gap: 12px; justify-content: center; margin: 25px auto; }
    .btn { width: 85px; height: 85px; font-size: 28px; font-weight: bold; background: #222; color: #fff; border: 2px solid #444; border-radius: 16px; touch-action: manipulation; cursor: pointer; }
    .btn:active { background: #00e676; color: #000; border-color: #00e676; }
    .slider-container { margin: 15px auto; width: 85%; max-width: 300px; background: #1e1e1e; padding: 15px; border-radius: 12px; }
    input[type=range] { width: 100%; accent-color: #00e676; }
  </style>
</head>
<body>
  <h2>ESP32 Web Control</h2>
  <div class="slider-container">
    <label>Speed: <b id="speedVal">180</b></label><br><br>
    <input type="range" min="80" max="255" value="180" id="speedSlider" oninput="updateSpeed(this.value)">
  </div>
  <div class="grid">
    <div></div>
    <button class="btn" onmousedown="sendCmd('F')" onmouseup="sendCmd('S')" ontouchstart="sendCmd('F')" ontouchend="sendCmd('S')">▲</button>
    <div></div>
    <button class="btn" onmousedown="sendCmd('L')" onmouseup="sendCmd('S')" ontouchstart="sendCmd('L')" ontouchend="sendCmd('S')">◄</button>
    <button class="btn" onmousedown="sendCmd('S')" ontouchstart="sendCmd('S')">■</button>
    <button class="btn" onmousedown="sendCmd('R')" onmouseup="sendCmd('S')" ontouchstart="sendCmd('R')" ontouchend="sendCmd('S')">►</button>
    <div></div>
    <button class="btn" onmousedown="sendCmd('B')" onmouseup="sendCmd('S')" ontouchstart="sendCmd('B')" ontouchend="sendCmd('S')">▼</button>
    <div></div>
  </div>

  <script>
    let lastCmd = "";
    function sendCmd(cmd) {
      if (cmd !== lastCmd) {
        lastCmd = cmd;
        fetch('/cmd?val=' + cmd, { cache: 'no-store' });
      }
    }
    function updateSpeed(val) {
      document.getElementById('speedVal').innerText = val;
      fetch('/speed?val=' + val, { cache: 'no-store' });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCmd() {
  if (server.hasArg("val")) {
    String cmd = server.arg("val");
    if (cmd == "F") moveForward();
    else if (cmd == "B") moveBackward();
    else if (cmd == "L") turnLeft();
    else if (cmd == "R") turnRight();
    else if (cmd == "S") stopCar();
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("val")) {
    currentSpeed = server.arg("val").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  stopCar();

  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Disable Wi-Fi power save to reduce HTTP request latency
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Setup Server Routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/speed", handleSpeed);

  server.begin();
  Serial.println("HTTP Server Started!");
}

void loop() {
  server.handleClient();
}