/*
  ESP32 Low-Latency UDP Steering Controller
  Wi-Fi AP: "ESP32_Car_WiFi" | Pass: "password123" | Port: 4210
*/

#include <WiFi.h>
#include <WiFiUdp.h>

// Motor Driver Pins (L298N)
const int ENA = 14;
const int IN1 = 27;
const int IN2 = 26;
const int IN3 = 25;
const int IN4 = 33;
const int ENB = 32;

int currentSpeed = 90; // Speed range: 0-255

// Access Point Credentials
const char* ssid = "ESP32_Car_WiFi";
const char* password = "password123";

// UDP Config
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

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  stopCar();

  // Initialize Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP Started! IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start UDP Listener
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
}