#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "webpage.h"

#define LED1 5         // GPIO5 (D1)
#define LED2 4         // GPIO4 (D2)
#define LED3 12        // GPIO12 (D6)
#define BUTTON 0       // GPIO0 (D3)
#define UART_RX_PIN 13 // RX = D7
#define UART_TX_PIN 15 // TX = D8

#define UART_BAUD 115200
#define DEBOUNCE_DELAY 50
#define UART_COMMAND 0x41           // 'A' у HEX
#define WIFI_SSID "ESP_Partner"
#define WIFI_PASSWORD "12345678"

SoftwareSerial mySerial(UART_RX_PIN, UART_TX_PIN); 
ESP8266WebServer server(80);
String lastStatus = "Партнер готовий";

bool ledStates[3] = {false, false, false};
int current = -1;
bool turnOn = true;

volatile unsigned long lastPress = 0;
volatile bool buttonPressed = false;


void ICACHE_RAM_ATTR handleButton() {
  unsigned long now = millis();
  if (now - lastPress > DEBOUNCE_DELAY) {
    buttonPressed = true;
    lastPress = now;
  }
}

// ==== ПЕРЕМИКАННЯ ДІОДІВ ====
void toggleNextLED() {
  current = (current + 1) % 3;
  ledStates[current] = turnOn;

  digitalWrite(LED1, ledStates[0]);
  digitalWrite(LED2, ledStates[1]);
  digitalWrite(LED3, ledStates[2]);

  lastStatus = String(turnOn ? "УВІМКНУТО " : "ВИМКНУТО ") + "діод #" + String(current + 1);
  turnOn = !turnOn;
}

void setup() {
  Serial.begin(UART_BAUD);
  mySerial.begin(UART_BAUD);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButton, FALLING);

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });

  server.on("/control", []() {
    String type = server.arg("type");

    if (type == "partner") {
      toggleNextLED();
      lastStatus = "Партнер: алгоритм виконано";
      server.send(200, "text/plain", lastStatus);
    }

    else if (type == "client") {
      mySerial.write(UART_COMMAND);  // Надсилаємо байт у HEX
      lastStatus = "Партнер: команда 0x41 (A) надіслана клієнту";
      server.send(200, "text/plain", lastStatus);
    }

    else {
      server.send(400, "text/plain", "Невідома команда");
    }
  });

  server.on("/status", []() {
    server.send(200, "text/plain", lastStatus);
  });

  server.begin();
  Serial.println("Партнер: WiFi AP ESP_Partner запущено");
}

void loop() {
  server.handleClient();

  // UART прийом байтів
  if (mySerial.available()) {
    byte received = mySerial.read();
    if (received == UART_COMMAND) {
      toggleNextLED();
      lastStatus = "Алгоритм активовано по UART (0x41)";
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    toggleNextLED();
    lastStatus = "Алгоритм активовано по кнопці";
  }
}
