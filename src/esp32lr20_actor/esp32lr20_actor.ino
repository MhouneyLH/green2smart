#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

const char* ssid = "sciencecamp04";
const char* wifiPassword = "camperfurt";
const char* mqttServer = "192.168.20.1";
const char* mqttClientId = "mqtt_esp32lr20";
const char* mqttUsername = "admin";
const char* mqttPassword = "root";
const int mqttPort = 1883;

const int RELAY_PIN_1 = 33;
const int RELAY_PIN_2 = 25;

const char* arduinoNanoTopic = "arduinoEnvironment/state";

unsigned long pumpPreviousMillis = 0;
const long pumpWaitInterval = 60000 * 60 * 3; // 3h
const long pumpInterval = 60000 * 7; // 7min
bool isPumpActive = false;

const int minBrightnessAsLume = 1400;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);

  while (!Serial);
  delay(5000);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageIncomingCallback);

  connectWiFi();
  connectMQTT();

  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);

  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);
}

void loop() {
  if(!client.connected()) {
    // Serial.println(client.connected());
    connectMQTT();
  }
  client.loop();
  // NO DELAY FUTURE LUCAS!

  const unsigned long pumpCurrentMillis = millis();
  if (!isPumpActive && isTimerOver(pumpCurrentMillis, pumpPreviousMillis, pumpWaitInterval)) {
    isPumpActive = true;
    digitalWrite(RELAY_PIN_1, HIGH);  

    pumpPreviousMillis = pumpCurrentMillis;
  }
  else if(isPumpActive && isTimerOver(pumpCurrentMillis, pumpPreviousMillis, pumpInterval)) {
    isPumpActive = false;
    digitalWrite(RELAY_PIN_1, LOW);

    pumpPreviousMillis = pumpCurrentMillis;
  }
}

bool isTimerOver(const unsigned long current, const unsigned long previous, const long interval) {
  return current - previous >= interval;
}

void onMessageIncomingCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived [");
  Serial.println(topic);
  Serial.println("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  if(String(topic) == arduinoNanoTopic) {
    char message[256]; // Buffer to store received message
    DynamicJsonDocument doc(1024); // Allocate memory for JSON parsing

    memcpy(message, payload, length);
    message[length] = '\0'; // Add null terminator

    // Parse the JSON message
    deserializeJson(doc, message);

    // Access and process the data from the JSON object
    JsonObject object = doc.as<JsonObject>();

    const float brightness = object["brightness"];
    if(!isEnoughLight(brightness)) {
      digitalWrite(RELAY_PIN_2, HIGH);
    }
    else {
      digitalWrite(RELAY_PIN_2, LOW);
    }
  }
  
  Serial.println();
}

bool isEnoughLight(const float brightness) {
  return brightness >= minBrightnessAsLume;
}

void connectWiFi() {
  WiFi.begin(ssid, wifiPassword);
  while (!wifiIsConnected()) {
    Serial.println("Connecting to WiFi..");
    delay(500);
  }

  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void connectMQTT() {
  while(!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(mqttClientId, mqttUsername, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(arduinoNanoTopic);
    } else {
      Serial.print("failed with error: ");
      Serial.print(client.state());
      delay(1000);
    }
  }
}