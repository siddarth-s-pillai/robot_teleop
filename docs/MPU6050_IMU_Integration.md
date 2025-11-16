#MPU6050 IMU Integration with NodeMCU v1
#Hardware Description • Wiring • Testing • Usage in Teleoperation Project
MPU6050 IMU Integration with NodeMCU v1
Hardware Description • Wiring • Testing • Usage in Teleoperation Project
#1. NodeMCU v1

The NodeMCU v1 is a compact, Wi-Fi–enabled microcontroller module based on the ESP8266 ESP-12E chip. It is designed for easy prototyping, offering built-in USB-to-serial programming and full compatibility with the Arduino IDE. The module integrates a 32-bit CPU, onboard Wi-Fi, and a flexible set of GPIO pins, making it suitable for IoT, sensing, and control applications.

#Key Features

2.4 GHz 802.11 b/g/n Wi-Fi with TCP/IP stack

32-bit Tensilica L106 CPU @ 80/160 MHz

Operates at 3.3 V logic

Interrupt-capable GPIOs, PWM, I²C, SPI, UART

Integrated 10-bit ADC

Supports station, access point, and AP+STA modes

Low-power operation with multiple sleep modes

Firmware upgrade support and Arduino-compatible development

#Pin Layout
<img width="707" height="755" alt="NodeMCU pinout" src="https://github.com/user-attachments/assets/fef82246-1c4d-41a9-a591-e929b3329b28" />
#2. MPU6050 IMU

The MPU6050 is a 6-axis motion tracking device that combines a 3-axis gyroscope and a 3-axis accelerometer on a single chip. It communicates with microcontrollers via the I²C protocol, making it ideal for orientation, motion detection, and inertial sensing in robotics and teleoperation projects.

#2.1 Functionality

Measures acceleration in X, Y, Z axes (ax, ay, az)

Measures angular velocity in X, Y, Z axes (gx, gy, gz)

Provides raw sensor data suitable for motion analysis, orientation estimation, and control loops.

#2.2 Pin Descriptions
<p float="left" align="center"> <img src="https://cdn.sparkfun.com//assets/parts/1/1/0/0/MPU6050_Pinout.png" width="300" height="220" /> </p>

VCC — Power supply (3.3V to 5V, NodeMCU uses 3.3V)

GND — Ground

SDA — I²C data line

SCL — I²C clock line

AD0 — I²C address selection (connect to GND for default 0x68)

INT — Optional interrupt output (not used in basic example)

#3. MPU6050 Integration With NodeMCU v1

This section explains how to connect the MPU6050 to NodeMCU, configure the Arduino IDE, upload the firmware, and acquire motion data.

#3.1 Hardware Connections
Wiring

MPU6050 Module             NodeMCU v1

VCC ---------------------> 3V3
GND ---------------------> GND
SDA ---------------------> D2  (GPIO4)
SCL ---------------------> D1  (GPIO5)
INT ---------------------> Not connected (optional)

Notes

NodeMCU uses 3.3V logic; connecting to 5V may damage the board.

Use short, shielded wires if possible to reduce noise on I²C lines.

Pull-up resistors (typically 4.7kΩ) may be required on SDA/SCL if your module does not include them.

#3.2 Arduino IDE Setup

Follow these steps to prepare your development environment:

Step 1 — Install Arduino IDE

Download from: https://www.arduino.cc/en/software

Step 2 — Add ESP8266 Board Package

1. Open File → Preferences
2. In Additional Board Manager URLs, add:http://arduino.esp8266.com/stable/package_esp8266com_index.json
3. http://arduino.esp8266.com/stable/package_esp8266com_index.json
4. Search for ESP8266 and install.

Step 3 — Select NodeMCU v1

Go to Tools → Board → NodeMCU 1.0 (ESP-12E Module)

Step 4 — Select COM Port

Go to Tools → Port → /dev/ttyUSB0 (Linux) or the relevant port on Windows/macOS.

#3.3 Firmware Example

Minimal MPU6050 Reading Code

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);  // SDA = D2 (GPIO4), SCL = D1 (GPIO5)

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful!");
  } else {
    Serial.println("MPU6050 connection FAILED!");
  }
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  Serial.print("Acc: ");
  Serial.print(ax); Serial.print(" ");
  Serial.print(ay); Serial.print(" ");
  Serial.print(az);

  Serial.print(" | Gyro: ");
  Serial.print(gx); Serial.print(" ");
  Serial.print(gy); Serial.print(" ");
  Serial.println(gz);

  delay(250);
}

#3.4 Observing & Acquiring Data
Observe Live Data

Open Tools → Serial Monitor

Set baud rate to 115200

Observe accelerometer and gyroscope values updating every 250 ms

Example output: 
Acc: -45 1023 16384 | Gyro: 12 -7 3
Acc: -44 1025 16383 | Gyro: 11 -6 2

#3.5 Data Logging & Usage

Use Serial Monitor → Save Output → CSV or TXT

Format for CSV: 
timestamp, ax, ay, az, gx, gy, gz
0.0, -45, 1023, 16384, 12, -7, 3
0.25, -44, 1025, 16383, 11, -6, 2

Data can be used for:

Orientation estimation

Motion tracking

Gesture or tilt detection

Teleoperation control loops

#3.6 Test Procedures
Test Table

| Test Case | Action                        | Expected Output                                 |
| --------- | ----------------------------- | ----------------------------------------------- |
| 1         | Power on MPU6050              | "MPU6050 connection successful!"                |
| 2         | Keep MPU6050 stationary       | Gyro ~ 0, Acc ~ [0,0,16384] (Z-axis gravity)    |
| 3         | Tilt board along X, Y, Z axes | Acc values change accordingly                   |
| 4         | Rotate board along axes       | Gyro values show positive/negative angular rate |
| 5         | Observe serial output         | Continuous streaming of Acc + Gyro values       |

