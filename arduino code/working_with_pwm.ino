/*
  ESP32 High-Speed Web Car Controller (Built-in Standard WebServer)
  - Creates Wi-Fi Access Point: "ESP32_Car_WiFi"
  - Password: "password123"
  - Open in phone browser: http://192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>

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

// Webpage UI (HTML + Responsive CSS + Touch JS)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <style>
    body { font-family: Arial; text-align: center; background-color: #1a1a1a; color: white; margin: 0; padding: 20px; user-select: none; }
    h2 { margin-bottom: 10px; }
    .grid { display: grid; grid-template-columns: repeat(3, 80px); gap: 10px; justify-content: center; margin: 20px auto; }
    .btn { width: 80px; height: 80px; font-size: 24px; font-weight: bold; background: #333; color: white; border: 2px solid #555; border-radius: 12px; touch-action: manipulation; }
    .btn:active { background: #008CBA; }
    .slider-container { margin: 20px auto; width: 80%; max-width: 300px; }
    input[type=range] { width: 100%; }
  </style>
</head>
<body>
  <h2>ESP32 Web Control</h2>
  <div class="slider-container">
    <label>Speed: <span id="speedVal">180</span></label><br>
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
    function sendCmd(cmd) {
      fetch('/cmd?val=' + cmd);
    }
    function updateSpeed(val) {
      document.getElementById('speedVal').innerText = val;
      fetch('/speed?val=' + val);
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
  WiFi.softAP(ssid, password);
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