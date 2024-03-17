#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// wifi
const char* SSID = "sciencecamp04";
const char* WIFI_PASSWORD = "camperfurt";

// mqtt
const char* MQTT_SERVER_ADRESS = "192.168.20.1";
const int MQTT_PORT = 1883;
const char* MQTT_USERNAME = "admin";
const char* MQTT_PASSWORD = "root";
const char* MQTT_CLIENT_ID = "mqtt_esp32lr20";

const char* ARDUINO_NANO_TOPIC_STATE = "arduinoEnvironment/state";

// relay
const int RELAY_PIN_1 = 33;
const int RELAY_PIN_2 = 25;

// pump
const long PUMP_INTERVAL_INACTIVE_AS_MS = 60000 * 60 * 3; // 3h
const long PUMP_INTERVAL_ACTIVE_AS_MS = 60000 * 7; // 7min
const float MIN_WATER_LEVEL = 20.0;
const float MAX_WATER_LEVEL = 100.0;

unsigned long pumpPreviousTimeAsMs = 0;
bool isPumpActive = false;

// light
const int MIN_BRIGHTNESS_AS_LUME = 1400;

// logging
const bool IS_PRINTING_INCOMING_MESSAGES = true;

// clients
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  // delay(5000);

  setupMQTT();
  setupPins();

  connectWiFi();
  connectMQTT();
}

void loop() {
  if(!client.connected()) {
    connectMQTT();
  }

  client.loop();
  handlePump();
}

void setupMQTT() {
  client.setServer(MQTT_SERVER_ADRESS, MQTT_PORT);
  client.setCallback(onMessageIncomingCallback);
}

void setupPins() {
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);

  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);
}

void connectWiFi() {
  WiFi.begin(SSID, WIFI_PASSWORD);
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
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(ARDUINO_NANO_TOPIC_STATE);
    } else {
      Serial.print("failed with error: ");
      Serial.print(client.state());
      delay(1000);
    }
  }
}

void handlePump() {
  const unsigned long pumpCurrentTimeAsMs = millis();

  if (!isPumpActive && isTimerOver(pumpCurrentTimeAsMs, pumpPreviousTimeAsMs, PUMP_INTERVAL_INACTIVE_AS_MS)) {
    activatePump();
    pumpPreviousTimeAsMs = pumpCurrentTimeAsMs;
  }
  else if(isPumpActive && isTimerOver(pumpCurrentTimeAsMs, pumpPreviousTimeAsMs, PUMP_INTERVAL_ACTIVE_AS_MS)) {
    deactivatePump();
    pumpPreviousTimeAsMs = pumpCurrentTimeAsMs;
  }
}

void activatePump() {
  isPumpActive = true;
  digitalWrite(RELAY_PIN_1, HIGH);
}

void deactivatePump() {
  isPumpActive = false;
  digitalWrite(RELAY_PIN_1, LOW);
}

bool isTimerOver(const unsigned long current, const unsigned long previous, const long interval) {
  return current - previous >= interval;
}

void onMessageIncomingCallback(char* topic, byte* payload, unsigned int length) {
  if(IS_PRINTING_INCOMING_MESSAGES) {
    printIncomingMessage();
  }

  if(String(topic) == ARDUINO_NANO_TOPIC_STATE) {
    const JsonObject jsonObject = getJSONObject(payload, length);
    const float brightness = jsonObject["brightness"];

    handleLight(brightness);
  }
}

void printIncomingMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived with topic [");
  Serial.print(topic);
  Serial.println("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
}

JsonObject getJSONObject(byte* payload, unsigned int length) {
  // TODO: adjust those numbers
  char messageBuffer[length + 1];
  DynamicJsonDocument doc(1024);

  memcpy(messageBuffer, payload, length);
  messageBuffer[length] = '\0';

  const DeserializationError error = deserializeJson(doc, messageBuffer);
  if(error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
  }

  const JsonObject object = doc.as<JsonObject>();
  return object;
}

void handleLight(const float brightness) {
  if(!isEnoughLight(brightness)) {
    digitalWrite(RELAY_PIN_2, HIGH);
    return;
  }

  digitalWrite(RELAY_PIN_2, LOW);
}

bool isEnoughLight(const float brightness) {
  return brightness >= MIN_BRIGHTNESS_AS_LUME;
}
