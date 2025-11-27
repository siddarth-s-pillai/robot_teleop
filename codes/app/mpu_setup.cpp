#include "mpu_setup.hpp"

MPU6050Wrapper::MPU6050Wrapper() {
  // Constructor body (can be empty if no initialization needed)
}

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
  return imuData;
}

void MPU6050Wrapper::updateIMUData() {
  mpu.getMotion6(&imuData.ax, &imuData.ay, &imuData.az,
                 &imuData.gx, &imuData.gy, &imuData.gz);
  lastReadTime = millis();
}