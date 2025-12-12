#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>
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
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_PIN 34    // Analog Input
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
float currentTemp = 0.0;
float currentHum = 0.0;
int currentMoisture = 0;
int currentLight = 0;

// Thresholds (Defaults)
float tempThreshold = 30.0;
int lightThreshold = 1000;
// Note: Pump/Moisture threshold is monitored but not actuated.

// Forward Declarations
void sendSensorData();
void controlActuators();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);

void setup() {
  Serial.begin(115200);
  
  // Init Sensors & Actuators
  dht.begin();
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
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int moisture = analogRead(SOIL_PIN);
  int light = analogRead(LDR_PIN);

  // Check if any reads failed and exit early (to avoid partial updates or nan)
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    // We can still send others? For now, let's skip.
    return;
  }
  
  // Update globals
  currentTemp = t;
  currentHum = h;
  currentMoisture = moisture;
  currentLight = light;

  Serial.printf("T:%.1f H:%.1f M:%d L:%d\n", t, h, moisture, light);

  // Send to Firebase
  Firebase.RTDB.setFloat(&fbdo, "/greenhouse/status/temp", t);
  Firebase.RTDB.setFloat(&fbdo, "/greenhouse/status/humidity", h);
  Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/moisture", moisture);
  Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/light", light);
}

void controlActuators() {
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
}

void streamCallback(FirebaseStream data) {
  Serial.printf("Stream Data Path: %s, Type: %s, Value: %s\n", 
                data.dataPath().c_str(), data.dataType().c_str(), data.stringData().c_str());

  if (data.dataType() == "int" || data.dataType() == "float" || data.dataType() == "double") {
    if (data.dataPath() == "/tempThreshold") {
      tempThreshold = data.floatData();
      Serial.printf("Updated tempThreshold: %.2f\n", tempThreshold);
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