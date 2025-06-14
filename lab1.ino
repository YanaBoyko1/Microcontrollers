#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Yana";
const char* password = "1234";

ESP8266WebServer server(80);

const int LED1_PIN = D4;
const int LED2_PIN = D6;
const int LED3_PIN = D7;
const int BUTTON_PIN = GPIO0;

const int ledPins[] = {LED1_PIN, LED2_PIN, LED3_PIN};
const int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

int currentLed = 0;
int direction = 1;
unsigned long lastLedChangeTime = 0;
const long ledInterval = 500;

volatile bool buttonPressed = false;
volatile unsigned long lastInterruptTime = 0;
unsigned long lastClickTime = 0;
const long debounceTime = 50;
const long doubleClickTime = 250;

void handleWebTrigger() {
  direction *= -1;
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleRoot() {
  String currentDirection = (direction == 1) ? "L1 -> L2 -> L3" : "L3 -> L2 -> L1";
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Керування LED</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; }
    h1 { color: #333; }
    p { font-size: 1.2em; }
    .btn {
      display: inline-block;
      padding: 15px 30px;
      font-size: 1.5em;
      cursor: pointer;
      text-align: center;
      text-decoration: none;
      outline: none;
      color: #fff;
      background-color: #4CAF50;
      border: none;
      border-radius: 15px;
      box-shadow: 0 9px #999;
    }
    .btn:hover { background-color: #3e8e41; }
    .btn:active {
      background-color: #3e8e41;
      box-shadow: 0 5px #666;
      transform: translateY(4px);
    }
  </style>
</head>
<body>
  <h1>Лабораторна робота №4</h1>
  <p>Поточний напрямок: <strong>)rawliteral";
  
  html += currentDirection;
  
  html += R"rawliteral(</strong></p>
  <br>
  <form action="/trigger" method="POST">
    <button type="submit" class="btn">Змінити напрямок</button>
  </form>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void IRAM_ATTR handleInterrupt() {
  if (millis() - lastInterruptTime > debounceTime) {
    buttonPressed = true;
    lastInterruptTime = millis();
  }
}

void handleDoubleClick() {
  if (buttonPressed) {
    unsigned long clickTime = millis();
    if (clickTime - lastClickTime < doubleClickTime) {
      direction *= -1;
      lastClickTime = 0;
    } else {
      lastClickTime = clickTime;
    }
    buttonPressed = false;
  }
}

void updateLeds() {
  if (millis() - lastLedChangeTime >= ledInterval) {
    lastLedChangeTime = millis();
    digitalWrite(ledPins[currentLed], LOW);
    currentLed = currentLed + direction;
    if (currentLed >= numLeds) {
      currentLed = 0;
    }
    if (currentLed < 0) {
      currentLed = numLeds - 1;
    }
    digitalWrite(ledPins[currentLed], HIGH);
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, FALLING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/trigger", HTTP_POST, handleWebTrigger);
  server.begin();
}

void loop() {
  server.handleClient();
  handleDoubleClick();
  updateLeds();
}
