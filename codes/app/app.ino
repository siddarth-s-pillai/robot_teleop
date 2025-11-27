#include "wifi_setup.hpp"
#include "ir_setup.hpp"
#include "RotaryEncoder.hpp"
#include "mpu_setup.hpp"

#define LED_PIN 2 // NodeMCU onboard LED (D4, GPIO2)
WiFiSetup wifiSetup;
IRSensor irSensor;
RotaryEncoder encoder(D5, D6, D7);
MPU6050Wrapper mpuWrapper;

char payload[] = "Hi there! Got the message :-)";

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn LED off initially (active LOW)
  bool res = false;
  bool flip = false;
  while (!res){
    digitalWrite(LED_PIN, flip ? LOW : HIGH);
    flip = !flip;
    res = wifiSetup.configureSoftAP("ESPsoftAP_01", "qwerty246810");
    delay(500);  
  }

  irSensor.begin();
  encoder.begin();
  mpuWrapper.begin();

  wifiSetup.printAndSendToAllConnectedClients(payload);
}

void loop() {
  int stations = wifiSetup.getNumConnections();
  Serial.printf("Stations connected = %d\n", stations);

  encoder.update();
  RotaryData enc = encoder.getRotaryEncoderData();

  mpuWrapper.updateIMUData();
  const IMUData& imuData = mpuWrapper.getIMUData();

  bool objectDetected = irSensor.isObjectDetected();
  char payload1[128];
  snprintf(payload1, sizeof(payload1), "Time: %lu; IR: %s; RE: %ld, %d, %s; MPU: %d, %d, %d, %d, %d, %d", millis(), objectDetected ? "H" : "L", enc.steps, enc.direction, enc.button ? "H": "L", imuData.ax, imuData.ay, imuData.az, imuData.gx, imuData.gy, imuData.gz);
  wifiSetup.printAndSendToAllConnectedClients(payload1);
  delay(10);
}
