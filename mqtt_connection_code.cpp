#include <WiFi.h>                // Use WiFi library for ESP32
#include <Adafruit_NeoPixel.h>    // Library for NeoPixel LED control
#include <DHT.h>                  // Library for DHT sensor
#include <ArduinoJson.h>          // Library for JSON parsing and serialization
#include <PubSubClient.h>         // MQTT client library

// --------------------------------------------------------------------------------------------
//        UPDATE CONFIGURATION TO MATCH YOUR ENVIRONMENT
// --------------------------------------------------------------------------------------------

// GPIO pin definitions
#define RGB_PIN 5 
#define RGB_PIN 17
#define RGB_PIN 18         // Single pin for NeoPixel RGB LED
#define DHT_PIN 4          // DHT sensor data pin

// Sensor and NeoPixel configuration
#define DHTTYPE DHT11
#define NEOPIXEL_TYPE NEO_RGB + NEO_KHZ800

// Temperature thresholds
const float ALARM_COLD = 0.0;
const float ALARM_HOT = 30.0;
const float WARN_COLD = 10.0;
const float WARN_HOT = 25.0;

// WiFi credentials
const char* ssid = "iphone";
const char* pass = "abhinay22";

// MQTT connection details
#define MQTT_HOST "mqtt.example.com"   // Replace with actual MQTT broker
#define MQTT_PORT 1883
#define MQTT_DEVICEID "esp32_sensor"   // Replace with a unique device identifier
#define MQTT_TOPIC_TEMP "temperature/status"
#define MQTT_TOPIC_HUMID "humidity/status"
#define MQTT_TOPIC_DISPLAY "display/cmd"

// --------------------------------------------------------------------------------------------
//       GLOBAL VARIABLES
// --------------------------------------------------------------------------------------------
Adafruit_NeoPixel pixel(1, RGB_PIN, NEOPIXEL_TYPE);
DHT dht(DHT_PIN, DHTTYPE);

// WiFi and MQTT client objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// --------------------------------------------------------------------------------------------
//       FUNCTION DECLARATIONS
// --------------------------------------------------------------------------------------------

void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool readSensorData(float &temperature, float &humidity);
void setLEDColor(float temperature);
void publishSensorData(float temperature, float humidity);
void connectToMQTT();

// --------------------------------------------------------------------------------------------
//       FUNCTIONS
// --------------------------------------------------------------------------------------------

// Initialize WiFi connection
void setupWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

// MQTT message callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  payload[length] = 0;  // Null-terminate the payload to treat as a string
  Serial.println((char*)payload);
}

// Function to read sensor data (returns true if successful)
bool readSensorData(float &temperature, float &humidity) {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return false;
  }
  return true;
}

// Set LED color based on temperature
void setLEDColor(float temperature) {
  uint8_t red = 0, green = 0, blue = 0;
  
  if (temperature <= ALARM_COLD) {
    blue = 255;       // Cold alarm
  } else if (temperature < WARN_COLD) {
    blue = 150;       // Cold warning
  } else if (temperature >= ALARM_HOT) {
    red = 255;        // Hot alarm
  } else if (temperature > WARN_HOT) {
    red = 150;        // Hot warning
  } else {
    green = 255;      // Safe temperature
  }
  
  pixel.setPixelColor(0, red, green, blue);
  pixel.show();
}

// Publish temperature and humidity data to MQTT topics
void publishSensorData(float temperature, float humidity) {
  StaticJsonDocument<100> jsonDoc;
  JsonObject status = jsonDoc.createNestedObject("status");
  status["temperature"] = temperature;
  status["humidity"] = humidity;
  
  char buffer[100];
  serializeJson(jsonDoc, buffer);

  if (!mqttClient.publish(MQTT_TOPIC_TEMP, buffer)) {
    Serial.println("Failed to publish temperature data");
  }
  if (!mqttClient.publish(MQTT_TOPIC_HUMID, buffer)) {
    Serial.println("Failed to publish humidity data");
  }
}

// Connect to MQTT broker
void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_DEVICEID)) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_TOPIC_DISPLAY);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// --------------------------------------------------------------------------------------------
//       MAIN PROGRAM
// --------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  setupWiFi();
  
  dht.begin();
  pixel.begin();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  connectToMQTT();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  float temperature, humidity;
  if (readSensorData(temperature, humidity)) {
    setLEDColor(temperature);
    publishSensorData(temperature, humidity);
  }

  // Poll MQTT for incoming messages every second
  for (int i = 0; i < 10; i++) {
    mqttClient.loop();
    delay(1000);
  }
}
