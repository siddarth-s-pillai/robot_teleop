#ifndef MPU_SETUP_HPP
#define MPU_SETUP_HPP

#include <Wire.h>
#include <MPU6050.h>

struct IMUData {
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
};

class MPU6050Wrapper {
public:
  MPU6050Wrapper();
  void begin();
  const IMUData& getIMUData();
  void updateIMUData();

private:
  MPU6050 mpu;
  IMUData imuData;
  unsigned long lastReadTime = 0;
};


#endif // MPU_SETUP_HPP
