#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/u_int32.hpp>

#include <cmath>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "teleop_telemetry_receiver/telemetry_packet.hpp"

namespace ttr = teleop_telemetry_receiver;

class TelemetryReceiverNode : public rclcpp::Node {
public:
  TelemetryReceiverNode() : Node("telemetry_receiver"), running_(true) {
    listen_address_ = declare_parameter<std::string>("listen_address", "0.0.0.0");
    listen_port_ = declare_parameter<int>("listen_port", 20025);
    frame_id_ = declare_parameter<std::string>("frame_id", "imu_link");

    accel_lsb_per_g_ = declare_parameter<double>("accel_lsb_per_g", 16384.0);
    gyro_lsb_per_dps_ = declare_parameter<double>("gyro_lsb_per_dps", 131.0);

    debug_rx_stats_ = declare_parameter<bool>("debug_rx_stats", true);
    debug_log_other_sizes_ = declare_parameter<bool>("debug_log_other_sizes", false);
    accept_text_packets_ = declare_parameter<bool>("accept_text_packets", true);
    accept_framed_packets_ = declare_parameter<bool>("accept_framed_packets", true);
    publish_gg_world_ = declare_parameter<bool>("publish_gg_world", true);
    compute_framed_angular_velocity_ = declare_parameter<bool>("compute_framed_angular_velocity", true);
    framed_gg_world_is_angular_velocity_ = declare_parameter<bool>("framed_gg_world_is_angular_velocity", true);

    imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("/telemetry/imu", rclcpp::SensorDataQoS());
    gg_world_pub_ = create_publisher<geometry_msgs::msg::Vector3Stamped>("/telemetry/gg_world", rclcpp::SensorDataQoS());
    mcu_ts_pub_ = create_publisher<std_msgs::msg::UInt32>("/telemetry/mcu_timestamp_ms", rclcpp::QoS(10));
    steps_pub_ = create_publisher<std_msgs::msg::Int32>("/telemetry/encoder_steps", rclcpp::QoS(10));
    direction_pub_ = create_publisher<std_msgs::msg::Int32>("/telemetry/encoder_direction", rclcpp::QoS(10));
    button_pub_ = create_publisher<std_msgs::msg::Bool>("/telemetry/encoder_button", rclcpp::QoS(10));
    ir_pub_ = create_publisher<std_msgs::msg::Bool>("/telemetry/ir_detected", rclcpp::QoS(10));

    socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
      throw std::runtime_error("Failed to create UDP socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(listen_port_));

    if (::inet_pton(AF_INET, listen_address_.c_str(), &addr.sin_addr) != 1) {
      ::close(socket_fd_);
      throw std::runtime_error("Invalid listen_address: " + listen_address_);
    }

    int reuse = 1;
    (void) ::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
      ::close(socket_fd_);
      throw std::runtime_error("Failed to bind UDP socket (addr=" + listen_address_ + ", port=" + std::to_string(listen_port_) + ")");
    }

    RCLCPP_INFO(get_logger(), "Listening UDP on %s:%d", listen_address_.c_str(), listen_port_);

    recv_thread_ = std::thread([this]() { this->recv_loop(); });
  }

  ~TelemetryReceiverNode() override {
    running_.store(false);

    if (socket_fd_ >= 0) {
      ::shutdown(socket_fd_, SHUT_RDWR);
      ::close(socket_fd_);
      socket_fd_ = -1;
    }

    if (recv_thread_.joinable()) {
      recv_thread_.join();
    }
  }

private:
  void recv_loop() {
    uint8_t buffer[512];

    uint64_t total_packets = 0;
    uint64_t telemetry_packets = 0;
    uint64_t text_packets = 0;
    uint64_t other_packets = 0;
    rclcpp::Time last_stats_time = this->get_clock()->now();

    while (rclcpp::ok() && running_.load()) {
      sockaddr_in src{};
      socklen_t srclen = sizeof(src);

      ssize_t n = ::recvfrom(socket_fd_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&src), &srclen);
      if (n <= 0) {
        continue;
      }

      total_packets++;

      const auto now = this->get_clock()->now();
      if (debug_rx_stats_ && (now - last_stats_time).seconds() >= 1.0) {
        last_stats_time = now;
        char src_ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &src.sin_addr, src_ip, sizeof(src_ip));
        RCLCPP_INFO(
          this->get_logger(),
          "rx stats: total=%lu telemetry=%lu text=%lu other=%lu last=%s:%u last_len=%ld",
          static_cast<unsigned long>(total_packets),
          static_cast<unsigned long>(telemetry_packets),
          static_cast<unsigned long>(text_packets),
          static_cast<unsigned long>(other_packets),
          src_ip,
          static_cast<unsigned int>(ntohs(src.sin_port)),
          static_cast<long>(n)
        );
      }

      if (static_cast<std::size_t>(n) != ttr::kTelemetryPacketSize) {
        if (accept_framed_packets_ && handle_framed_packet(buffer, static_cast<std::size_t>(n))) {
          text_packets++;
          continue;
        }
        if (accept_text_packets_ && handle_text_payload(buffer, static_cast<std::size_t>(n))) {
          text_packets++;
          continue;
        }

        other_packets++;
        if (debug_log_other_sizes_) {
          char src_ip[INET_ADDRSTRLEN] = {0};
          ::inet_ntop(AF_INET, &src.sin_addr, src_ip, sizeof(src_ip));
          RCLCPP_WARN(this->get_logger(), "ignored packet from %s:%u len=%ld (expected %zu)",
                      src_ip, static_cast<unsigned int>(ntohs(src.sin_port)), static_cast<long>(n), ttr::kTelemetryPacketSize);
        }
        continue;
      }

      ttr::TelemetryPacket pkt{};
      if (!ttr::parse_telemetry_packet(buffer, static_cast<std::size_t>(n), pkt)) {
        continue;
      }

      telemetry_packets++;

      publish(pkt);
    }
  }

  void publish(const ttr::TelemetryPacket &pkt) {
    const auto now = this->get_clock()->now();

    // Publish IMU (raw -> SI). Orientation is unknown for this packet.
    sensor_msgs::msg::Imu imu_msg;
    imu_msg.header.stamp = now;
    imu_msg.header.frame_id = frame_id_;

    // Unknown orientation: set covariance[0] = -1 to indicate "not provided".
    imu_msg.orientation_covariance[0] = -1.0;

    const double deg2rad = M_PI / 180.0;
    const double g_ms2 = 9.80665;

    imu_msg.angular_velocity.x = (static_cast<double>(pkt.gx) / gyro_lsb_per_dps_) * deg2rad;
    imu_msg.angular_velocity.y = (static_cast<double>(pkt.gy) / gyro_lsb_per_dps_) * deg2rad;
    imu_msg.angular_velocity.z = (static_cast<double>(pkt.gz) / gyro_lsb_per_dps_) * deg2rad;

    imu_msg.linear_acceleration.x = (static_cast<double>(pkt.ax) / accel_lsb_per_g_) * g_ms2;
    imu_msg.linear_acceleration.y = (static_cast<double>(pkt.ay) / accel_lsb_per_g_) * g_ms2;
    imu_msg.linear_acceleration.z = (static_cast<double>(pkt.az) / accel_lsb_per_g_) * g_ms2;

    imu_pub_->publish(imu_msg);

    std_msgs::msg::UInt32 mcu_ts;
    mcu_ts.data = pkt.timestamp_ms;
    mcu_ts_pub_->publish(mcu_ts);

    std_msgs::msg::Int32 steps;
    steps.data = pkt.steps;
    steps_pub_->publish(steps);

    std_msgs::msg::Int32 direction;
    direction.data = pkt.direction;
    direction_pub_->publish(direction);

    std_msgs::msg::Bool button;
    button.data = (pkt.button != 0);
    button_pub_->publish(button);

    std_msgs::msg::Bool ir;
    ir.data = (pkt.ir_detected != 0);
    ir_pub_->publish(ir);
  }

  static std::string trim_copy(std::string s) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.front()))) {
      s.erase(s.begin());
    }
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.back()))) {
      s.pop_back();
    }
    return s;
  }

  // Accepts common text payloads used in this repo:
  // 1) app/app.ino debug line (single line):
  //    "Time: %lu; IR: H|L; RE: <steps>, <dir>, H|L; MPU: ax, ay, az, gx, gy, gz\n"
  // 2) MPU6050_DMP6-style lines (may arrive as one or many UDP datagrams):
  //    "Time: <ms>" / "quat\t..." / "aa\t..." / "IR:\tL" / "RE:\t..."
  bool handle_text_payload(const uint8_t *data, std::size_t len) {
    // Ensure it's reasonably printable.
    std::string text(reinterpret_cast<const char *>(data), len);
    for (char &c : text) {
      unsigned char uc = static_cast<unsigned char>(c);
      if (uc == '\n' || uc == '\r' || uc == '\t') {
        continue;
      }
      if (uc < 0x20 || uc > 0x7E) {
        return false;
      }
    }

    bool handled_any = false;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
      line = trim_copy(line);
      if (line.empty()) {
        continue;
      }
      handled_any |= handle_text_line(line);
    }
    // If it was a single line without \n, still try it.
    if (!handled_any) {
      handled_any = handle_text_line(trim_copy(text));
    }
    return handled_any;
  }

  bool handle_text_line(const std::string &line) {
    // app/app.ino format (has semicolons)
    if (line.find("Time:") != std::string::npos && line.find("MPU:") != std::string::npos) {
      return parse_app_line(line);
    }

    // DMP6-ish lines
    if (line.rfind("Time:", 0) == 0) {
      // Time: 56387
      std::istringstream ss(line);
      std::string key;
      uint32_t t = 0;
      ss >> key >> t;
      if (t != 0 || line.find('0') != std::string::npos) {
        text_state_.timestamp_ms = t;
        return true;
      }
      return false;
    }

    if (line.rfind("quat", 0) == 0) {
      // quat\t1.00\t0.00\t0.00\t-0.01
      std::istringstream ss(line);
      std::string tag;
      double w, x, y, z;
      ss >> tag >> w >> x >> y >> z;
      if (!ss.fail()) {
        text_state_.quat_wxyz = {w, x, y, z};
        maybe_publish_dmp6_imu();
        return true;
      }
      return false;
    }

    if (line.rfind("aa", 0) == 0) {
      // aa\t0.00\t-0.00\t9.80   (already m/s^2)
      std::istringstream ss(line);
      std::string tag;
      double ax, ay, az;
      ss >> tag >> ax >> ay >> az;
      if (!ss.fail()) {
        text_state_.aa_ms2 = {ax, ay, az};
        maybe_publish_dmp6_imu();
        return true;
      }
      return false;
    }

    if (line.rfind("IR:", 0) == 0) {
      // IR:  L
      const bool detected = (line.find('H') != std::string::npos || line.find('1') != std::string::npos);
      text_state_.ir_detected = detected;
      std_msgs::msg::Bool ir;
      ir.data = detected;
      ir_pub_->publish(ir);
      return true;
    }

    if (line.rfind("RE:", 0) == 0) {
      // RE:  0  0  L
      std::istringstream ss(line);
      std::string tag;
      long steps = 0;
      long dir = 0;
      std::string btn;
      ss >> tag >> steps >> dir >> btn;
      if (!ss.fail()) {
        text_state_.steps = static_cast<int32_t>(steps);
        text_state_.direction = static_cast<int32_t>(dir);
        text_state_.button = (!btn.empty() && (btn[0] == 'H' || btn[0] == '1'));

        std_msgs::msg::Int32 steps_msg;
        steps_msg.data = text_state_.steps.value();
        steps_pub_->publish(steps_msg);

        std_msgs::msg::Int32 dir_msg;
        dir_msg.data = text_state_.direction.value();
        direction_pub_->publish(dir_msg);

        std_msgs::msg::Bool button;
        button.data = text_state_.button.value();
        button_pub_->publish(button);
        return true;
      }
      return false;
    }

    return false;
  }

  bool parse_app_line(const std::string &line) {
    // Example:
    // Time: 123; IR: H; RE: 100, 1, L; MPU: 1, 2, 3, 4, 5, 6
    auto find_after = [&](const std::string &key) -> std::optional<std::string> {
      const auto pos = line.find(key);
      if (pos == std::string::npos) {
        return std::nullopt;
      }
      return line.substr(pos + key.size());
    };

    uint32_t ts = 0;
    {
      const auto after = find_after("Time:");
      if (!after) return false;
      std::istringstream ss(*after);
      ss >> ts;
      if (ss.fail()) return false;
    }

    bool ir = false;
    {
      const auto after = find_after("IR:");
      if (!after) return false;
      ir = (after->find('H') != std::string::npos || after->find('1') != std::string::npos);
    }

    int32_t steps = 0;
    int32_t dir = 0;
    bool button = false;
    {
      const auto after = find_after("RE:");
      if (!after) return false;
      // Steps and dir are before commas
      char comma;
      std::istringstream ss(*after);
      ss >> steps >> comma >> dir >> comma;
      if (ss.fail()) return false;
      std::string btn;
      ss >> btn;
      button = (!btn.empty() && (btn[0] == 'H' || btn[0] == '1'));
    }

    int ax, ay, az, gx, gy, gz;
    {
      const auto after = find_after("MPU:");
      if (!after) return false;
      char comma;
      std::istringstream ss(*after);
      ss >> ax >> comma >> ay >> comma >> az >> comma >> gx >> comma >> gy >> comma >> gz;
      if (ss.fail()) return false;
    }

    ttr::TelemetryPacket pkt{};
    pkt.timestamp_ms = ts;
    pkt.steps = steps;
    pkt.direction = dir;
    pkt.ax = static_cast<int16_t>(ax);
    pkt.ay = static_cast<int16_t>(ay);
    pkt.az = static_cast<int16_t>(az);
    pkt.gx = static_cast<int16_t>(gx);
    pkt.gy = static_cast<int16_t>(gy);
    pkt.gz = static_cast<int16_t>(gz);
    pkt.button = static_cast<uint8_t>(button ? 1 : 0);
    pkt.ir_detected = static_cast<uint8_t>(ir ? 1 : 0);
    publish(pkt);
    return true;
  }

  void maybe_publish_dmp6_imu() {
    if (!text_state_.quat_wxyz || !text_state_.aa_ms2) {
      return;
    }

    sensor_msgs::msg::Imu imu_msg;
    imu_msg.header.stamp = this->get_clock()->now();
    imu_msg.header.frame_id = frame_id_;

    imu_msg.orientation.w = text_state_.quat_wxyz->w;
    imu_msg.orientation.x = text_state_.quat_wxyz->x;
    imu_msg.orientation.y = text_state_.quat_wxyz->y;
    imu_msg.orientation.z = text_state_.quat_wxyz->z;

    imu_msg.linear_acceleration.x = text_state_.aa_ms2->x;
    imu_msg.linear_acceleration.y = text_state_.aa_ms2->y;
    imu_msg.linear_acceleration.z = text_state_.aa_ms2->z;

    // Angular velocity not provided by these lines.
    imu_msg.angular_velocity_covariance[0] = -1.0;

    imu_pub_->publish(imu_msg);

    if (text_state_.timestamp_ms) {
      std_msgs::msg::UInt32 mcu_ts;
      mcu_ts.data = *text_state_.timestamp_ms;
      mcu_ts_pub_->publish(mcu_ts);
    }
  }

  static uint16_t crc16_ccitt_false(const uint8_t *data, std::size_t len) {
    // CRC-16/CCITT-FALSE: init 0xFFFF, poly 0x1021
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < len; ++i) {
      crc ^= static_cast<uint16_t>(data[i]) << 8;
      for (int b = 0; b < 8; ++b) {
        if (crc & 0x8000) {
          crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
        } else {
          crc = static_cast<uint16_t>(crc << 1);
        }
      }
    }
    return crc;
  }

  static float read_f32_le(const uint8_t *p) {
    uint32_t u = (static_cast<uint32_t>(p[0])) |
                 (static_cast<uint32_t>(p[1]) << 8) |
                 (static_cast<uint32_t>(p[2]) << 16) |
                 (static_cast<uint32_t>(p[3]) << 24);
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
  }

  static uint32_t read_u32_le(const uint8_t *p) {
    return (static_cast<uint32_t>(p[0])) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
  }

  static int32_t read_i32_le(const uint8_t *p) {
    return static_cast<int32_t>(read_u32_le(p));
  }

  static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  }

  struct Quatf {
    float w;
    float x;
    float y;
    float z;
  };

  static Quatf quat_conj(const Quatf &q) {
    return Quatf{q.w, -q.x, -q.y, -q.z};
  }

  static Quatf quat_mul(const Quatf &a, const Quatf &b) {
    return Quatf{
      a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
      a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
      a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
      a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
    };
  }

  // Supports framed packets like the one you showed:
  // aa 55 01 3f [63-byte payload] [2-byte CRC]
  // This commonly matches a "DMP" payload:
  //   u32 timestamp_ms
  //   float quat[4]
  //   float aa_ms2[3]
  //   float gg_world[3]
  //   float ypr[3]
  //   i32 steps
  //   u8  button
  //   u8  ir
  //   u8  reserved
  bool handle_framed_packet(const uint8_t *data, std::size_t len) {
    if (!data || len < 6) {
      return false;
    }
    if (data[0] != 0xAA || data[1] != 0x55) {
      return false;
    }

    const uint8_t msg_id = data[2];
    const uint8_t payload_len = data[3];

    const std::size_t expected_total = static_cast<std::size_t>(payload_len) + 4 + 2;
    if (len != expected_total) {
      return false;
    }

    // Optional CRC check (silently ignore if it doesn't match; firmware may use different CRC).
    const uint16_t crc_expected = static_cast<uint16_t>(data[len - 2]) | (static_cast<uint16_t>(data[len - 1]) << 8);
    const uint16_t crc_calc = crc16_ccitt_false(data, len - 2);
    (void)crc_expected;
    (void)crc_calc;

    if (msg_id != 0x01) {
      return false;
    }
    if (payload_len != 0x3F) {
      // Only accept the observed 63-byte payload for now.
      return false;
    }

    const uint8_t *p = data + 4;
    std::size_t off = 0;

    const uint32_t timestamp_ms = read_u32_le(p + off);
    off += 4;

    const Quatf q{read_f32_le(p + off), read_f32_le(p + off + 4), read_f32_le(p + off + 8), read_f32_le(p + off + 12)};
    off += 16;

    const float ax = read_f32_le(p + off); off += 4;
    const float ay = read_f32_le(p + off); off += 4;
    const float az = read_f32_le(p + off); off += 4;

    const float ggx = read_f32_le(p + off); off += 4;
    const float ggy = read_f32_le(p + off); off += 4;
    const float ggz = read_f32_le(p + off); off += 4;

    // ypr (ignored for now)
    off += 12;

    // Remaining 7 bytes (63 - 56): steps(int32) + direction(int8) + button(uint8) + ir(uint8)
    const int32_t steps = read_i32_le(p + off);
    off += 4;

    const int8_t direction = static_cast<int8_t>(*(p + off));
    off += 1;

    const uint8_t button = *(p + off);
    off += 1;

    const uint8_t ir = *(p + off);
    off += 1;

    if (off != payload_len) {
      return false;
    }

    // Publish IMU with orientation + linear accel.
    sensor_msgs::msg::Imu imu_msg;
    imu_msg.header.stamp = this->get_clock()->now();
    imu_msg.header.frame_id = frame_id_;
    imu_msg.orientation.w = q.w;
    imu_msg.orientation.x = q.x;
    imu_msg.orientation.y = q.y;
    imu_msg.orientation.z = q.z;
    imu_msg.linear_acceleration.x = ax;
    imu_msg.linear_acceleration.y = ay;
    imu_msg.linear_acceleration.z = az;

    // In the ElectronicCats MPU6050 DMP examples, "ggWorld" is world-frame gyro.
    // If your firmware sends ggWorld in rad/s (as this repo's app now does), use it directly.
    if (framed_gg_world_is_angular_velocity_) {
      imu_msg.angular_velocity.x = ggx;
      imu_msg.angular_velocity.y = ggy;
      imu_msg.angular_velocity.z = ggz;
    } else {
      // Otherwise estimate angular velocity from quaternion delta.
      if (compute_framed_angular_velocity_ && last_framed_.has_value()) {
        const uint32_t prev_ms = last_framed_->timestamp_ms;
        const float dt = (timestamp_ms > prev_ms) ? (static_cast<float>(timestamp_ms - prev_ms) * 1e-3f) : 0.0f;
        if (dt > 1e-4f && dt < 1.0f) {
          // q_delta = q_curr * conj(q_prev)
          Quatf q_delta = quat_mul(q, quat_conj(last_framed_->q));
          // Ensure shortest rotation.
          if (q_delta.w < 0.0f) {
            q_delta.w = -q_delta.w;
            q_delta.x = -q_delta.x;
            q_delta.y = -q_delta.y;
            q_delta.z = -q_delta.z;
          }

          const float w_clamped = clampf(q_delta.w, -1.0f, 1.0f);
          const float half_angle = std::acos(w_clamped);
          const float sin_half = std::sin(half_angle);

          float axis_x = 0.0f;
          float axis_y = 0.0f;
          float axis_z = 0.0f;
          if (std::fabs(sin_half) > 1e-6f) {
            axis_x = q_delta.x / sin_half;
            axis_y = q_delta.y / sin_half;
            axis_z = q_delta.z / sin_half;
          }

          const float angle = 2.0f * half_angle;
          const float omega = angle / dt;

          imu_msg.angular_velocity.x = axis_x * omega;
          imu_msg.angular_velocity.y = axis_y * omega;
          imu_msg.angular_velocity.z = axis_z * omega;
        } else {
          imu_msg.angular_velocity_covariance[0] = -1.0;
        }
      } else {
        imu_msg.angular_velocity_covariance[0] = -1.0;
      }
    }

    imu_pub_->publish(imu_msg);

    last_framed_ = FramedState{timestamp_ms, q};

    if (publish_gg_world_) {
      geometry_msgs::msg::Vector3Stamped g;
      g.header.stamp = imu_msg.header.stamp;
      g.header.frame_id = frame_id_;
      g.vector.x = ggx;
      g.vector.y = ggy;
      g.vector.z = ggz;
      gg_world_pub_->publish(g);
    }

    std_msgs::msg::UInt32 mcu_ts;
    mcu_ts.data = timestamp_ms;
    mcu_ts_pub_->publish(mcu_ts);

    std_msgs::msg::Int32 steps_msg;
    steps_msg.data = steps;
    steps_pub_->publish(steps_msg);

    std_msgs::msg::Int32 dir_msg;
    dir_msg.data = static_cast<int32_t>(direction);
    direction_pub_->publish(dir_msg);

    std_msgs::msg::Bool button_msg;
    button_msg.data = (button != 0);
    button_pub_->publish(button_msg);

    std_msgs::msg::Bool ir_msg;
    ir_msg.data = (ir != 0);
    ir_pub_->publish(ir_msg);

    return true;
  }

  std::string listen_address_;
  int listen_port_{};
  std::string frame_id_;
  double accel_lsb_per_g_{};
  double gyro_lsb_per_dps_{};

  bool debug_rx_stats_{true};
  bool debug_log_other_sizes_{false};
  bool accept_text_packets_{true};
  bool accept_framed_packets_{true};
  bool publish_gg_world_{true};
  bool compute_framed_angular_velocity_{true};
  bool framed_gg_world_is_angular_velocity_{true};

  struct FramedState {
    uint32_t timestamp_ms;
    Quatf q;
  };
  std::optional<FramedState> last_framed_;

  struct {
    std::optional<uint32_t> timestamp_ms;
    std::optional<int32_t> steps;
    std::optional<int32_t> direction;
    std::optional<bool> button;
    std::optional<bool> ir_detected;
    struct Quat { double w, x, y, z; };
    struct Vec3 { double x, y, z; };
    std::optional<Quat> quat_wxyz;
    std::optional<Vec3> aa_ms2;
  } text_state_;

  int socket_fd_{-1};
  std::atomic<bool> running_;
  std::thread recv_thread_;

  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3Stamped>::SharedPtr gg_world_pub_;
  rclcpp::Publisher<std_msgs::msg::UInt32>::SharedPtr mcu_ts_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr steps_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr direction_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr button_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr ir_pub_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TelemetryReceiverNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
