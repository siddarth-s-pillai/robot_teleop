#include "wifi_setup.hpp"
#include "ir_setup.hpp"
#include "RotaryEncoder.hpp"
#include "mpu_setup.hpp"
#include "packet_data.hpp"

#define LED_PIN 2 // NodeMCU onboard LED (D4, GPIO2)
WiFiSetup wifiSetup;
IRSensor irSensor;
RotaryEncoder encoder(D5, D6, D7);
MPU6050Wrapper mpuWrapper;
unsigned long time_interval = 30; // millisec
TelemetryPacket packet;
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
  unsigned long time_start = millis();
  int stations = wifiSetup.getNumConnections();
  Serial.printf("Stations connected = %d\n", stations);

  encoder.update();
  RotaryData enc = encoder.getRotaryEncoderData();

  mpuWrapper.updateIMUData();
  const IMUData& imuData = mpuWrapper.getIMUData();

  bool objectDetected = irSensor.isObjectDetected();
  char payload1[128];
  snprintf(payload1, sizeof(payload1), "Time: %lu; IR: %s; RE: %ld, %d, %s; MPU: %d, %d, %d, %d, %d, %d\n", millis(), objectDetected ? "H" : "L", enc.steps, enc.direction, enc.button ? "H": "L", imuData.ax, imuData.ay, imuData.az, imuData.gx, imuData.gy, imuData.gz);
  // wifiSetup.printAndSendToAllConnectedClients(payload1);
  packet.timestamp = millis();
  packet.steps     = (int32_t)enc.steps;
  packet.direction = (int32_t)enc.direction;
  
  // Casting to int16_t explicitly to match packet size
  packet.ax = (int16_t)imuData.ax;
  packet.ay = (int16_t)imuData.ay;
  packet.az = (int16_t)imuData.az;
  packet.gx = (int16_t)imuData.gx;
  packet.gy = (int16_t)imuData.gy;
  packet.gz = (int16_t)imuData.gz;
  
  packet.button      = enc.button ? 1 : 0;
  packet.ir_detected = objectDetected ? 1 : 0;

  wifiSetup.sendBinaryToAllConnectedClients((uint8_t*)&packet, sizeof(packet));

  // Debug Output (Optional, for Serial Monitor)
  // Serial.printf("Stations: %d | Time: %lu | IR: %d | Packet Size: %d\n", 
  //               stations, millis(), packet.ir_detected, sizeof(packet));

  Serial.printf(payload1);

  unsigned long sleep_duration = time_interval - (millis() - time_start);
  sleep_duration = (sleep_duration > 0) ? sleep_duration : 0;

  delay(sleep_duration);
}
