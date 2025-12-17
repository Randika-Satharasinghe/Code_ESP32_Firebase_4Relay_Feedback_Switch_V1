void setup() {
  // Initialize digital pin 2 as an output.
  pinMode(2, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
  Serial.println("LED ON");
  delay(1000);             // wait for a second
  digitalWrite(2, LOW);    // turn the LED off (LOW is the voltage level)
  Serial.println("LED OFF");
  delay(1000);             // wait for a second
}
