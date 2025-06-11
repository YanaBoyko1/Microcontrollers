
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


const char* ssid = "YanaiPhone";
const char* password = "123456"; 

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_publish_topic = "wemos/d1/soil_moisture";
const char* mqtt_command_topic = "wemos/d1/commands";
const char* mqtt_client_id = "wemos-soil-sensor-final";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

char lastMessage[50] = "No commands";
unsigned long previousMillis = 0;
const long interval = 2000;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("--> MQTT Message Received <--");
  Serial.print("Topic: ");
  Serial.println(topic);

  if (length < sizeof(lastMessage)) {
    memcpy(lastMessage, payload, length);
    lastMessage[length] = '\0';
    Serial.print("Message: ");
    Serial.println(lastMessage);
  }
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("OK! Connected to MQTT.");
      client.subscribe(mqtt_command_topic);
      Serial.print("Subscribed to command topic: ");
      Serial.println(mqtt_command_topic);
    } else {
      Serial.print("FAILED, rc=");
      Serial.print(client.state());
      Serial.println(". Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n\n--- Starting Final Firmware ---");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,25);
  display.print("Starting...");
  display.display();
  Serial.println("Setup: Display OK.");

  Serial.print("Setup: Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK!");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Setup: MQTT client configured.");
  
  Serial.println("--- Setup Complete. Entering main loop. ---");
  delay(1000);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    int moisturePercent = map(analogRead(A0), 780, 315, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);
    Serial.print("Sensor value: ");
    Serial.println(moisturePercent);
    char msg[4];
    snprintf(msg, 4, "%d", moisturePercent);
    client.publish(mqtt_publish_topic, msg, true);
    Serial.println("Published sensor data to MQTT.");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Vologist:");
    display.setTextSize(2);
    display.setCursor(35, 12);
    display.print(moisturePercent);
    display.print("%");
    display.drawLine(0, 32, 127, 32, WHITE);
    display.setTextSize(1);
    display.setCursor(0, 36);
    display.print("Last command:");
    display.setTextSize(2);
    display.setCursor(0, 48);
    display.print(lastMessage);
    display.display();
    Serial.println("Display updated.");
    Serial.println("--------------------");
  }
}