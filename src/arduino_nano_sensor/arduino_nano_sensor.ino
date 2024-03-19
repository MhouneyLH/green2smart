#include <Arduino_LPS22HB.h>
#include <Arduino_HS300x.h>
#include <Arduino_APDS9960.h>
#include <ArduinoJson.h>

void setup() {
  Serial.begin(9600);

  while (!Serial);

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
  }
}

void loop() {
  const float pressure = BARO.readPressure();
  const float temperature = HS300x.readTemperature();
  const float humidity = HS300x.readHumidity();

  while (!APDS.colorAvailable()) {
    delay(5);
  }

  int r, g, b;
  APDS.readColor(r, g, b);
  const float brightnessAsLuma = 0.2126 * r + 0.7152 * g + 0.0722 * b;

  DynamicJsonDocument doc(255);
  JsonObject jsonObject = doc.to<JsonObject>();

  jsonObject["temp"] = String(temperature, 2);
  jsonObject["humidity"] = String(humidity, 2);
  jsonObject["pressure"] = String(pressure, 2);
  jsonObject["microphone"] = String(100, 2);
  jsonObject["brightness"] = String(brightnessAsLuma, 2);

  String payload;
  serializeJson(doc, payload);

  Serial.print("SENSOR_START");
  Serial.print(payload);
  Serial.println("SENSOR_END");

  delay(1000);
}