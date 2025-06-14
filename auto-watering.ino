#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Slk2.4Ghz";
const char* password = "20121978";
const char* serverUrl = "http://192.168.0.139:3000/api/moisture";
const char* controlUrl = "http://192.168.0.139:3000/api/status";

const int relayPin = 5; // GPIO5
const int moistureThreshold = 600; // Порог вологості (налаштуй під свій датчик)

unsigned long lastSensorSend = 0;
unsigned long lastRelayCheck = 0;

const unsigned long sensorInterval = 2000; // 2 секунди
const unsigned long relayInterval = 1000;  // 1 секунда

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Насос вимкнено

  WiFi.begin(ssid, password);
  Serial.println("Підключення до WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("✅ WiFi підключено");
  Serial.print("IP-адреса: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    int moisture = analogRead(A0);
    if (now - lastSensorSend >= sensorInterval) {
      lastSensorSend = now;

      Serial.print("Вологість ґрунту: ");
      Serial.println(moisture);

      HTTPClient http;
      http.begin(client, serverUrl);
      http.addHeader("Content-Type", "application/json");

      String payload = "{\"value\":" + String(moisture) + "}";
      int httpCode = http.POST(payload);

      if (httpCode > 0) {
        Serial.print("✅ Дані відправлено! HTTP код: ");
        Serial.println(httpCode);
      } else {
        Serial.print("❌ Помилка при надсиланні: ");
        Serial.println(http.errorToString(httpCode));
      }
      http.end();
    }

    if (now - lastRelayCheck >= relayInterval) {
      lastRelayCheck = now;

      HTTPClient controlHttp;
      controlHttp.begin(client, controlUrl);
      int controlCode = controlHttp.GET();

      if (controlCode == HTTP_CODE_OK) {
        String response = controlHttp.getString();
        Serial.print("📥 Відповідь сервера /api/status: ");
        Serial.println(response);

        bool serverOn = (response.indexOf("\"state\":\"on\"") != -1);
        bool soilDry = (moisture > moistureThreshold);

        // Логіка: вмикаємо насос, якщо сухо або сервер наказує увімкнути
        if (serverOn || soilDry) {
          digitalWrite(relayPin, LOW);  // Вмикаємо насос
          Serial.println("🔌 Реле: ВМИКНЕНО");
        } else {
          digitalWrite(relayPin, HIGH); // Вимикаємо насос
          Serial.println("🔌 Реле: ВИМКНЕНО");
        }
      } else {
        Serial.print("❌ Не вдалося отримати статус реле: ");
        Serial.println(controlHttp.errorToString(controlCode));
      }
      controlHttp.end();
    }
  } else {
    Serial.println("⚠️ WiFi втрачено! Спроба перепідключення...");
    WiFi.begin(ssid, password);
  }
}
