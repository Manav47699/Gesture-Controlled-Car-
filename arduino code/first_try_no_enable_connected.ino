/*
  ESP32 Classic Bluetooth Motor Control
  Works directly with phone System Bluetooth settings & standard Serial Bluetooth Apps.
*/

#include "BluetoothSerial.h"

// Check if Bluetooth is properly configured on board
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

// Pin Definitions
const int IN1 = 27; // Left Motor Forward
const int IN2 = 26; // Left Motor Backward
const int IN3 = 25; // Right Motor Forward
const int IN4 = 33; // Right Motor Backward

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void setup() {
  Serial.begin(115200);

  // Set motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ensure car is stopped initially
  stopCar();

  // Initialize Classic Bluetooth with device name
  SerialBT.begin("ESP32_Car"); 
  Serial.println("Bluetooth Started! Scan for 'ESP32_Car' in Phone Settings.");
}

void loop() {
  if (SerialBT.available()) {
    char command = SerialBT.read();
    Serial.print("Received Command: ");
    Serial.println(command);

    // Direction Commands
    if (command == 'F' || command == 'f') {
      moveForward();
    } 
    else if (command == 'B' || command == 'b') {
      moveBackward();
    } 
    else if (command == 'L' || command == 'l') {
      turnLeft();
    } 
    else if (command == 'R' || command == 'r') {
      turnRight();
    } 
    else if (command == 'S' || command == 's') {
      stopCar();
    }
  }
}