#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <ESP32Servo.h>

// ==========================================
//  YOUR CREDENTIALS
// ==========================================
#define WIFI_SSID "A1_7313"
#define WIFI_PASSWORD "6DDHH4A7AC"

#define API_KEY "AIzaSyACy6ePjitOl0YisxlPgPSij394OE2OcZg"
#define DATABASE_URL "https://esp-32-relay-01-5bb14-default-rtdb.firebaseio.com/"
#define USER_EMAIL "abc@gmail.com"
#define USER_PASSWORD "123123"

// ==========================================
//  PIN DEFINITIONS & SENSOR OBJECTS
// ==========================================
// Capacitive Soil Moisture Sensor
#define SOIL_PIN 34           // Analog Input (ADC1)
const int AirValue = 3250;    // Dry reading (calibrated)
const int WaterValue = 1420;  // Wet reading (calibrated)

#define LDR_PIN 35     // Analog Input
#define RELAY_FAN 5
#define RELAY_LIGHT 18
#define SERVO_PIN 19

Servo myservo;

// ==========================================
//  GLOBALS
// ==========================================
FirebaseData fbdo;
FirebaseData streamData; // For streaming config
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

// Current Sensor Readings
int currentMoistureRaw = 0;
int currentMoisturePercent = 0;
int currentLight = 0;

// Thresholds (Defaults)
int moistureThreshold = 30; // Percentage
int lightThreshold = 1000;

// Forward Declarations
void sendSensorData();
void controlActuators();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);

void setup() {
  Serial.begin(115200);
  
  // Init Sensors & Actuators
  // Configure ADC for capacitive sensor
  analogReadResolution(12);     // 12-bit resolution (0-4095)
  analogSetAttenuation(ADC_11db); // Allow reading up to 3.3V
  
  myservo.attach(SERVO_PIN);
  myservo.write(0); // init closed

  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_LIGHT, OUTPUT);
  
  digitalWrite(RELAY_FAN, LOW); 
  digitalWrite(RELAY_LIGHT, LOW); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Initialize stream for Config
  if (!Firebase.RTDB.beginStream(&streamData, "/greenhouse/config")) {
    Serial.printf("Stream begin error, %s\n", streamData.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
}

void loop() {
  // Sensor Reporting & Control Cycle (Every 5 seconds)
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    sendSensorData();
    controlActuators();
  }
}

void sendSensorData() {
  // Read capacitive soil moisture sensor
  int moistureRaw = analogRead(SOIL_PIN);
  int light = analogRead(LDR_PIN);

  // Convert moisture to percentage (reversed: high value = dry)
  int moisturePercent = map(moistureRaw, AirValue, WaterValue, 0, 100);
  
  // Constrain to 0-100%
  if (moisturePercent > 100) moisturePercent = 100;
  else if (moisturePercent < 0) moisturePercent = 0;
  
  // Update globals
  currentMoistureRaw = moistureRaw;
  currentMoisturePercent = moisturePercent;
  currentLight = light;

  Serial.printf("Moisture: %d%% (Raw: %d) | Light: %d\n", moisturePercent, moistureRaw, light);

  // 1. Send Real-time Status (Overwrites existing)
  Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/moisturePercent", moisturePercent);
  Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/moistureRaw", moistureRaw);
  Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/light", light);

  // 2. Push History Entry (Append new node)
  FirebaseJson json;
  json.set("moisturePercent", moisturePercent);
  json.set("moistureRaw", moistureRaw);
  json.set("light", light);
  json.set("ts/.sv", "timestamp"); // Firebase server timestamp

  if (Firebase.RTDB.pushJSON(&fbdo, "/greenhouse/history", &json)) {
      Serial.println("History data pushed");
  } else {
      Serial.println(fbdo.errorReason());
  }
}

void controlActuators() {
  // Actuators Disabled as per request
  /*
  // 1. Fan/Vent Logic
  if (currentTemp > tempThreshold) {
    digitalWrite(RELAY_FAN, HIGH);
    myservo.write(90); // Open
    Serial.println("Fan ON, Vent OPEN");
  } else {
    digitalWrite(RELAY_FAN, LOW);
    myservo.write(0);  // Closed
    Serial.println("Fan OFF, Vent CLOSED");
  }

  // 2. Grow Light Logic
  if (currentLight < lightThreshold) {
    digitalWrite(RELAY_LIGHT, HIGH);
    Serial.println("Light ON");
  } else {
    digitalWrite(RELAY_LIGHT, LOW);
    Serial.println("Light OFF");
  }
  */
  Serial.println("Actuators disabled.");
}

void streamCallback(FirebaseStream data) {
  Serial.printf("Stream Data Path: %s, Type: %s, Value: %s\n", 
                data.dataPath().c_str(), data.dataType().c_str(), data.stringData().c_str());

  if (data.dataType() == "int" || data.dataType() == "float" || data.dataType() == "double") {
    if (data.dataPath() == "/moistureThreshold") {
      moistureThreshold = data.intData();
      Serial.printf("Updated moistureThreshold: %d\n", moistureThreshold);
    } else if (data.dataPath() == "/lightThreshold") {
      lightThreshold = data.intData();
      Serial.printf("Updated lightThreshold: %d\n", lightThreshold);
    }
  }
  // Trigger control immediately on config change
  controlActuators();
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, resuming...");
  }
}