// ESP32 + MH Sensor Series "Flying Fish" IR Sensor Test
// - Reads digital output from sensor
// - Prints status to Serial Monitor
// - Turns onboard LED on when object detected

#define LED_PIN       2   // Onboard LED (on most ESP32 dev boards)
#define SENSOR_PIN    4   // IR sensor OUT/DO pin connected here

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== IR Sensor Test Starting ===");
  Serial.println("Make sure:");
  Serial.println("- VCC -> 3.3V");
  Serial.println("- GND -> GND");
  Serial.println("- OUT/DO -> GPIO 4");
  Serial.println("-------------------------------");

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);  // Module usually has its own pull-up

  digitalWrite(LED_PIN, LOW);  // LED off initially
}

void loop() {
  int sensorValue = digitalRead(SENSOR_PIN);

  // Most 'flying fish' IR obstacle sensors:
  //  - OUTPUT LOW (0) when OBJECT is detected
  //  - OUTPUT HIGH (1) when NO object
  bool objectDetected = (sensorValue == LOW);

  if (objectDetected) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Object DETECTED (sensor LOW)");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("No object (sensor HIGH)");
  }

  delay(200);  // small delay to avoid spamming Serial
}
