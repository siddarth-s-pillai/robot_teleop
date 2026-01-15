#pragma once

#include <cstdint>
#include <cstddef>

namespace teleop_telemetry_receiver {

struct TelemetryPacket {
  uint32_t timestamp_ms;
  int32_t steps;
  int32_t direction;
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
  uint8_t button;
  uint8_t ir_detected;
};

static constexpr std::size_t kTelemetryPacketSize = 26;

inline uint16_t read_u16_le(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

inline uint32_t read_u32_le(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

inline int16_t read_i16_le(const uint8_t *data) {
  return static_cast<int16_t>(read_u16_le(data));
}

inline int32_t read_i32_le(const uint8_t *data) {
  return static_cast<int32_t>(read_u32_le(data));
}

inline bool parse_telemetry_packet(const uint8_t *data, std::size_t len, TelemetryPacket &out) {
  if (!data || len != kTelemetryPacketSize) {
    return false;
  }

  std::size_t offset = 0;
  out.timestamp_ms = read_u32_le(data + offset);
  offset += 4;

  out.steps = read_i32_le(data + offset);
  offset += 4;

  out.direction = read_i32_le(data + offset);
  offset += 4;

  out.ax = read_i16_le(data + offset); offset += 2;
  out.ay = read_i16_le(data + offset); offset += 2;
  out.az = read_i16_le(data + offset); offset += 2;

  out.gx = read_i16_le(data + offset); offset += 2;
  out.gy = read_i16_le(data + offset); offset += 2;
  out.gz = read_i16_le(data + offset); offset += 2;

  out.button = *(data + offset); offset += 1;
  out.ir_detected = *(data + offset); offset += 1;

  return offset == kTelemetryPacketSize;
}

}  // namespace teleop_telemetry_receiver
