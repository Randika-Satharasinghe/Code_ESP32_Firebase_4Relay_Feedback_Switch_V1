// --- USER CONFIGURATION ---
const int sensorPin = 34;    // Use an ADC1 pin (32-36, 39)
const int AirValue = 3250;   // REPLACE with your "Dry" reading
const int WaterValue = 1420; // REPLACE with your "Wet" reading
// --------------------------

int soilMoistureValue = 0;
int soilMoisturePercent = 0;

void setup() {
  Serial.begin(115200);
  
  // Set resolution to 12-bit (0-4095)
  analogReadResolution(12);
  // Set attenuation to allow reading up to 3.3V
  analogSetAttenuation(ADC_11db); 
}

void loop() {
  soilMoistureValue = analogRead(sensorPin);
  
  // Map the raw value to percentage
  // Explanation: map(value, fromLow, fromHigh, toLow, toHigh)
  // Because the sensor works in reverse (High Value = Dry), we map:
  // AirValue -> 0%
  // WaterValue -> 100%
  soilMoisturePercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  // Fix edge cases:
  // Sometimes readings go slightly beyond calibrated values, 
  // causing results like -5% or 105%. We constrain them to 0-100.
  if(soilMoisturePercent > 100) {
    soilMoisturePercent = 100;
  } else if(soilMoisturePercent < 0) {
    soilMoisturePercent = 0;
  }

  // Print results
  Serial.print("Raw: ");
  Serial.print(soilMoistureValue);
  Serial.print("  |  Moisture: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");

  delay(1000);
}