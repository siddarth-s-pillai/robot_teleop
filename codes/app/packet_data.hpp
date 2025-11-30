// packet_data.h
#ifndef PACKET_DATA_H
#define PACKET_DATA_H

#include <stdint.h>

// __attribute__((packed)) prevents compiler padding
struct __attribute__((packed)) TelemetryPacket {
    uint32_t timestamp;     // Time (millis)
    int32_t steps;          // Encoder steps
    int32_t direction;      // Encoder direction
    int16_t ax, ay, az;     // MPU6050 Accel (Raw int16)
    int16_t gx, gy, gz;     // MPU6050 Gyro (Raw int16)
    uint8_t button;         // Encoder button (1 = High, 0 = Low)
    uint8_t ir_detected;    // IR Sensor (1 = Detected, 0 = Not)
};

#endif
