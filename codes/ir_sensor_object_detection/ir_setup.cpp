#include "ir_setup.hpp"


void IRSensor::begin() {
  pinMode(sensor_pin, INPUT);
}

void IRSensor::updateSensorState() {
    int sensorValue = digitalRead(SENSOR_PIN);
    objectDetected = (sensorValue == LOW);
    lastReadTime = millis();
}

bool IRSensor::isObjectDetected() {
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime >= 10) { 
        updateSensorState();
    }
  return objectDetected;
}