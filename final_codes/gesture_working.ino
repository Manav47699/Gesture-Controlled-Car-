/*
  ESP32 Low-Latency UDP Steering Controller (STABILIZED)
*/

// this is esp code. this will classify fist and plam. fist is stearing wheel and palm is reverse.

#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>

// Motor Driver Pins (L298N)
const int ENA = 14;
const int IN1 = 27;
const int IN2 = 26;
const int IN3 = 25;
const int IN4 = 33;
const int ENB = 32;

int currentSpeed = 90; // Speed range: 0-255

const char* ssid = "ESP32_Car_WiFi";
const char* password = "password123";

WiFiUDP udp;
const unsigned int localUdpPort = 4210;
char packetBuffer[10];

void setSpeed(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void moveForward()  { setSpeed(currentSpeed); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void moveBackward() { setSpeed(currentSpeed); digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void turnLeft()     { setSpeed(currentSpeed); digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void turnRight()    { setSpeed(currentSpeed); digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void stopCar()      { setSpeed(0);            digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  stopCar();

  // 1. Initialize Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  // 2. Disable Wi-Fi Sleep Mode (Prevents AP dropping)
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.print("AP Started! IP Address: ");
  Serial.println(WiFi.softAPIP());

  // 3. Start UDP Listener
  udp.begin(localUdpPort);
  Serial.printf("UDP Server listening on port %d\n", localUdpPort);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      char cmd = packetBuffer[0];
      
      switch (cmd) {
        case 'F': moveForward();  break;
        case 'B': moveBackward(); break;
        case 'L': turnLeft();     break;
        case 'R': turnRight();    break;
        case 'S': stopCar();      break;
        default:  stopCar();      break;
      }
    }
  }
  
  // Yield to keep system tasks & Wi-Fi stack stable
  vTaskDelay(1); 
}