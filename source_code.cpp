#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --------------------------------------------------------------------------------------------
//        UPDATE CONFIGURATION TO MATCH YOUR ENVIRONMENT
// --------------------------------------------------------------------------------------------

// Define GPIO pins
#define RGB_PIN 5
#define RGB_PIN 17
#define RGB_PIN 18
#define DHT_PIN 4

// Define sensor type and NeoPixel configuration
#define DHTTYPE DHT11
#define NEOPIXEL_TYPE NEO_RGB + NEO_KHZ800

// Define temperature thresholds for LED color change
const float ALARM_COLD = 0.0;
const float ALARM_HOT = 30.0;
const float WARN_COLD = 10.0;
const float WARN_HOT = 25.0;

// WiFi connection details
const char* ssid = "iphone";
const char* pass = "abhinay22";

// --------------------------------------------------------------------------------------------
//       GLOBAL VARIABLES
// --------------------------------------------------------------------------------------------
Adafruit_NeoPixel pixel(1, RGB_PIN, NEOPIXEL_TYPE);
DHT dht(DHT_PIN, DHTTYPE);

StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");

float humidity = 0.0;
float temperature = 0.0;

// --------------------------------------------------------------------------------------------
//       FUNCTIONS
// --------------------------------------------------------------------------------------------

// Function to set up WiFi connection
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

// Function to read sensor values and handle error if failed
bool readSensorData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return false;
  }
  return true;
}

// Function to set LED color based on temperature thresholds
void setLEDColor() {
  uint8_t red = 0, green = 0, blue = 0;

  if (temperature < ALARM_COLD) {
    blue = 255;
  } else if (temperature < WARN_COLD) {
    blue = 150;
  } else if (temperature > ALARM_HOT) {
    red = 255;
  } else if (temperature > WARN_HOT) {
    red = 150;
  } else if (temperature > WARN_COLD && temperature <= WARN_HOT) {
    green = 255;
  }

  pixel.setPixelColor(0, red, green, blue);
  pixel.show();
}

// Function to print data in JSON format
void printJSONData() {
  status["temp"] = temperature;
  status["humidity"] = humidity;

  char msg[50];
  serializeJson(jsonDoc, msg, sizeof(msg));
  Serial.println(msg);
}

// --------------------------------------------------------------------------------------------
//       MAIN PROGRAM
// --------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("ESP8266 Sensor Application");

  setupWiFi();

  dht.begin();
  pixel.begin();
}

void loop() {
  if (readSensorData()) {
    setLEDColor();
    printJSONData();
  }
  delay(10000);
}
