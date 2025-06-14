#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "webpage.h"

#define LED1 5     // GPIO5 (D1)
#define LED2 4     // GPIO4 (D2)
#define LED3 12    // GPIO12 (D6)
#define BUTTON 0   // GPIO0 (D3)

#define LED_INTERVAL 300
#define COUNT_LEDS 3
#define DEBOUNCE_DELAY 50
#define DOUBLE_CLICK_DELAY 300

#define UART_RX_PIN 13  
#define UART_TX_PIN 15  

SoftwareSerial mySerial(UART_RX_PIN, UART_TX_PIN);
ESP8266WebServer server(80);

unsigned long previousMillis = 0;
int currentLed = 0;
bool reverseMode = false;
String lastStatus = "Клієнт готовий";

volatile unsigned long lastPress = 0;
volatile int clickCount = 0;
unsigned long lastClickTime = 0;

int leds[] = {LED1, LED2, LED3};

void ICACHE_RAM_ATTR handleButtonInterrupt() {
  unsigned long now = millis();
  if (now - lastPress > DEBOUNCE_DELAY) {
    clickCount++;
    lastPress = now;
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);

  for (int i = 0; i < COUNT_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButtonInterrupt, FALLING);

  WiFi.softAP("ESP_Client", "12345678");

  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });

  server.on("/control", []() {
    String type = server.arg("type");
    if (type == "client") {
      reverseMode = !reverseMode;
      currentLed = reverseMode ? COUNT_LEDS - 1 : 0;
      lastStatus = "Клієнт: напрямок " + String(reverseMode ? "←" : "→");
      previousMillis = millis();
      server.send(200, "text/plain", lastStatus);
    } else if (type == "partner") {
      mySerial.write(0x41);  // Надіслати HEX-біт (A)
      lastStatus = "Команда 0x41 (HEX) надіслана партнеру";
      server.send(200, "text/plain", lastStatus);
    } else {
      server.send(400, "text/plain", "Невідома команда");
    }
  });

  server.on("/status", []() {
    server.send(200, "text/plain", lastStatus);
  });

  server.begin();
  Serial.println("Клієнт: Готово. WiFi AP: ESP_Client");

  digitalWrite(leds[currentLed], HIGH);
}

void loop() {
  server.handleClient();

  // UART: прийом HEX-байта
  if (mySerial.available()) {
    byte received = mySerial.read();
    if (received == 0x41) {  // HEX для 'A'
      reverseMode = !reverseMode;
      currentLed = reverseMode ? COUNT_LEDS - 1 : 0;
      lastStatus = "Алгоритм активовано по UART (0x41)";
      previousMillis = millis();
    }
  }

  // Подвійне натискання кнопки
  if (clickCount == 2 && millis() - lastClickTime > DOUBLE_CLICK_DELAY) {
    reverseMode = !reverseMode;
    currentLed = reverseMode ? COUNT_LEDS - 1 : 0;
    lastStatus = "Напрямок змінено по кнопці";
    previousMillis = millis();
    clickCount = 0;
    lastClickTime = millis();
  }

  // Автоматичне переключення світлодіодів
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= LED_INTERVAL) {
    previousMillis = currentMillis;

    digitalWrite(leds[currentLed], LOW);

    if (reverseMode) {
      currentLed = (currentLed - 1 + COUNT_LEDS) % COUNT_LEDS;
    } else {
      currentLed = (currentLed + 1) % COUNT_LEDS;
    }

    digitalWrite(leds[currentLed], HIGH);
  }
}
