#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "FastLED.h"
#include "credentials.h"

// How many leds are in the strip?
#define NUM_LEDS (237)
#define CENTER_LED_INDEX_LEFT (116)
#define CENTER_LED_INDEX_RIGHT (CENTER_LED_INDEX_LEFT + 1)

// Data pin that led data will be written out over
#define DATA_PIN 4

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

const char* mqtt_server = "192.168.228.242";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[31];
int value = 0;


void setup() {
  Serial.begin(115200);
  delay(1000);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  
  fill_solid(&(leds[0]), NUM_LEDS, CRGB::Black);
  leds[0] = CRGB(0, 0, 255);
  leds[CENTER_LED_INDEX_LEFT] = CRGB(0, 255, 255);
  leds[CENTER_LED_INDEX_RIGHT] = CRGB(255, 0, 255);
  leds[NUM_LEDS - 1] = CRGB(255, 0, 0);
  FastLED.show();
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  fill_solid(&(leds[0]), NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void setup_wifi() {
    delay(10);

    scanNetworks();
    WiFi.begin(ssid, password);
    
    int status = WiFi.status();
    while (status != WL_CONNECTED) {
        Serial.println(status);        
        delay(3000);
        status = WiFi.status();
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void scanNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") Network: ");
    Serial.println(WiFi.SSID(thisNet));
  }
}

// warm white - no brightness
CHSV currentColor = CHSV(21, 183, 0);
CHSV targetColor = CHSV(21, 183, 80); 

void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }


  if (String(topic) == "Felix/LEDStrip/PowerState") {
    if (messageTemp != "true") {
      targetColor.value = 0;
    } else {
      targetColor.value = 255;
    }
  } else if (String(topic) == "Felix/LEDStrip/Hue") {
    targetColor.hue = map(messageTemp.toInt(), 0, 360, 0, 255);
    
  } else if (String(topic) == "Felix/LEDStrip/Saturation") {
    targetColor.saturation = map(messageTemp.toInt(), 0, 100, 0, 255);
    
  } else if (String(topic) == "Felix/LEDStrip/Brightness") {
    targetColor.value = map(messageTemp.toInt(), 0, 100, 0, 255);
    
  } else {
    Serial.print("Unrecognized topic '");
    Serial.print(topic);
    Serial.print("'. Message: '");
    Serial.print(messageTemp);
    Serial.println("'");
  }
  
  Serial.print("Hue: ");
  Serial.print(targetColor.hue);
  Serial.print(" | Saturation: ");
  Serial.print(targetColor.saturation);
  Serial.print(" | Brightness: ");
  Serial.println(targetColor.value);
  
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("Felix/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long lastTime = 0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long currentTime = millis();
  if (currentTime - lastTime > 20) {
    updateStrip();
    lastTime = currentTime;
  }
}



void updateStrip() {
  
    // shift all pixels by one
    for (int i = 0; i < CENTER_LED_INDEX_LEFT; ++i) {
        leds[i] = leds[i + 1];
    }
    for (int i = NUM_LEDS - 1; i > CENTER_LED_INDEX_RIGHT; --i) {
        leds[i] = leds[i - 1];
    }

    //adjust center pixels' color a little
    
    int hueOffset = (int)targetColor.hue - (int)currentColor.hue;
    int saturationOffset = (int)targetColor.saturation - (int)currentColor.saturation;
    int valueOffset = (int)targetColor.value - (int)currentColor.value;
    
    /*
    Serial.print("Offsets -> Hue: ");
    Serial.print(hueOffset);
    Serial.print(" | Saturation: ");
    Serial.print(saturationOffset);
    Serial.print(" | Brightness: ");
    Serial.println(valueOffset);
    */

    if (abs(hueOffset) <= 2) {
        currentColor.hue = targetColor.hue;
    } else {
      if (hueOffset > 128 || (hueOffset < 0 && hueOffset > -128)) {
        //lets go clockwise (-)
          currentColor.hue -= constrain((abs(hueOffset) % 128) / 16, 1, 8);
      } else {
          currentColor.hue += constrain((abs(hueOffset) % 128) / 16, 1, 8);
      }
    }
    
    if (abs(saturationOffset) <= 2) {
        currentColor.saturation = targetColor.saturation;
    } else {
        currentColor.saturation += (saturationOffset < 0 ? -1 : 1) * constrain(abs(saturationOffset) / 16, 1, 8);
    }
    
    if (abs(valueOffset) <= 2) {
        currentColor.value = targetColor.value;
    } else {
      
        currentColor.value += (valueOffset < 0 ? -1 : 1) * constrain(abs(valueOffset) / 16, 1, 8);
    }
    
    leds[CENTER_LED_INDEX_LEFT] = currentColor;
    leds[CENTER_LED_INDEX_RIGHT] = currentColor;
    FastLED.show();
}

