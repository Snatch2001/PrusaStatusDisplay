#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

const char* ssid = "Heink";
const char* password = "16363936504648121340";
const char* server = "192.168.178.172";
const int port = 80;
const String pathPrinter = "/api/printer";
const String pathJob = "/api/job";
const char* apiKey = "kDfSQLzhMbTMrRa";

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 5000;

#define LED_PIN 4
#define NUM_LEDS 28
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

bool isPrinting = false;
bool attentionState = false; 
unsigned long lastBlinkTime = 0;
bool isYellow = false; 

int brightness = 128;

void setup() {
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.begin(115200);
  delay(100);
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;

    isPrinting = checkPrintingState() == 1;
    attentionState = getLinkState();

    if (isPrinting && attentionState) {
        if (currentTime - lastBlinkTime >= 1000) { 
            isYellow = !isYellow; 
            lastBlinkTime = currentTime;
        }
        if (isYellow) {
            setAllLEDsColor(strip.Color(255, 255, 0)); 
        } else {
            setBlueLEDs(getCompletion()); 
        }
    } else if (isPrinting) {
        setBlueLEDs(getCompletion());
    } else {
        setAllLEDsColor(strip.Color(255, 255, 255)); 
    }
  }
}

void setAllLEDsColor(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

int checkPrintingState() {
  HTTPClient http;
  http.begin(server, port, pathPrinter);
  http.addHeader("X-Api-Key", apiKey);
  int httpCode = http.GET();

  if (httpCode <= 0) {
      Serial.println("Error accessing printer state.");
      return -1;
  }

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    bool printing = doc["state"]["flags"]["printing"];
    http.end();
    return printing ? 1 : 0;
  }

  return -1;
}

bool getLinkState() {
  HTTPClient http;
  http.begin(server, port, pathPrinter);
  http.addHeader("X-Api-Key", apiKey);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String linkState = doc["state"]["flags"]["link_state"].as<String>();
    http.end();
    return (linkState == "ATTENTION" || linkState == "PAUSED");
  }
  return false;
}

float getCompletion() {
  HTTPClient http;
  http.begin(server, port, pathJob);
  http.addHeader("X-Api-Key", apiKey);
  int httpCode = http.GET();

  if (httpCode <= 0) {
      Serial.println("Error accessing job completion.");
      return -1.0f;
  }

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    float completion = doc["progress"]["completion"];
    http.end();
    return completion;
  }

  return -1.0f;
}

void setBlueLEDs(float completion) {
  int numLitLEDs = int(completion * NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < numLitLEDs) {
        strip.setPixelColor(i, strip.Color(0, 0, 255)); // Blau
    } else {
        strip.setPixelColor(i, strip.Color(255, 255, 255)); // Weiß
    }
  }
  strip.show();
}
