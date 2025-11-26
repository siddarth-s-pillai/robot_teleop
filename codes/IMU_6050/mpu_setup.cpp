#include "mpu_setup.hpp"

void MPU6050Wrapper::begin() {
  Wire.begin(D2, D1);  // SDA, SCL
  mpu.initialize();
  
  // Serial must be started in main setup() before calling this
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful!");
  } else {
    Serial.println("MPU6050 connection FAILED!");
  }
}


const IMUData& MPU6050Wrapper::getIMUData() {
  unsigned long currentTime = millis();
  
  // Logic check: 10ms is very fast, make sure that's intended. 
  if (currentTime - lastReadTime >= 30) { 
    updateIMUData();
  }
  return imuData;
}

void MPU6050Wrapper::updateIMUData() {
  mpu.getMotion6(&imuData.ax, &imuData.ay, &imuData.az,
                 &imuData.gx, &imuData.gy, &imuData.gz);
  lastReadTime = millis();
}