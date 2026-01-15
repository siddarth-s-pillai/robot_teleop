# teleop_telemetry_receiver

ROS 2 node that receives the ESP8266 `TelemetryPacket` (26 bytes) over UDP and republishes it.

## Publishes
- `/telemetry/imu` (`sensor_msgs/msg/Imu`)
  - `orientation` not provided (covariance[0] = -1)
  - `angular_velocity` from raw gyro, scaled to rad/s
  - `linear_acceleration` from raw accel, scaled to m/s^2 (not gravity-compensated)
- `/telemetry/gg_world` (`geometry_msgs/msg/Vector3Stamped`)
  - Gravity vector from framed (DMP) packets, if present. (This is not gyro.)
- `/telemetry/mcu_timestamp_ms` (`std_msgs/msg/UInt32`)
- `/telemetry/encoder_steps` (`std_msgs/msg/Int32`)
- `/telemetry/encoder_direction` (`std_msgs/msg/Int32`)
- `/telemetry/encoder_button` (`std_msgs/msg/Bool`)
- `/telemetry/ir_detected` (`std_msgs/msg/Bool`)

## Parameters
- `listen_address` (string, default `0.0.0.0`)
- `listen_port` (int, default `20025`)
- `frame_id` (string, default `imu_link`)
- `accel_lsb_per_g` (double, default `16384.0`) — MPU6050 @ ±2g
- `gyro_lsb_per_dps` (double, default `131.0`) — MPU6050 @ ±250 dps
- `publish_gg_world` (bool, default `true`) — publish `/telemetry/gg_world` for framed packets
- `compute_framed_angular_velocity` (bool, default `true`) — estimate `/telemetry/imu.angular_velocity` from quaternion delta for framed packets

## Notes
- This receiver supports multiple on-wire formats:
  - 26-byte binary `TelemetryPacket` from `app/packet_data.hpp`.
  - 69-byte framed packet with header `0xAA 0x55 0x01 0x3F ... CRC16` (seen in your captures). This publishes:
    - `orientation` from quaternion (float32)
    - `linear_acceleration` from `aa` (float32, m/s^2)
    - `/telemetry/gg_world` from `ggWorld` (float32)
    - `angular_velocity` is not in the frame; the node can estimate it from successive quaternions (or mark it as not provided)
