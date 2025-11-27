#include "ir_setup.hpp"

IRSensor::IRSensor() {
  // Constructor body (can be empty)
}

void IRSensor::begin() {
  pinMode(sensor_pin, INPUT);
  updateSensorState();
}

void IRSensor::updateSensorState() {
    int sensorValue = digitalRead(SENSOR_PIN);
    objectDetected = (sensorValue == LOW);
    lastReadTime = millis();
}

bool IRSensor::isObjectDetected() {
    unsigned long currentTime = millis();
    // if (currentTime - lastReadTime >= 10) { 
    //     updateSensorState();
    // }
    updateSensorState();
    return objectDetected;
}