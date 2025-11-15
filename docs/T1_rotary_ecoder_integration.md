# Rotary Encoder Integration with NodeMCU v1  
### Hardware Description • Wiring • Testing • Usage in Teleoperation Project

---

## 1. NodeMCU v1 
The ** NodeMCU v1** is a compact, Wi-Fi–enabled microcontroller module based on the **ESP8266 ESP-12E** chip. It is designed for easy prototyping, offering built-in USB-to-serial programming and full compatibility with the Arduino IDE. The module integrates a 32-bit CPU, onboard Wi-Fi, and a flexible set of GPIO pins, making it suitable for IoT, sensing, and control applications.

---

### **Key Features**

* 2.4 GHz **802.11 b/g/n Wi-Fi** with TCP/IP stack
* **32-bit Tensilica L106 CPU** @ 80/160 MHz
* Operates at **3.3 V logic**
* **Interrupt-capable GPIOs**, PWM, I²C, SPI, UART
* Integrated **10-bit ADC**
* Supports **station, access point, and AP+STA** modes
* Low-power operation with multiple sleep modes
* Firmware upgrade support and Arduino-compatible development


### Pin Layout

<img width="707" height="755" alt="image" src="https://github.com/user-attachments/assets/fef82246-1c4d-41a9-a591-e929b3329b28" />


---

## 2. Rotary Encoder
The module used is a **mechanical incremental quadrature rotary encoder** with two output channels (**A** and **B**) and an optional **push-button** (switch).

### 2.1 Functionality
- Converts rotational motion into digital pulses.  
- Produces two square waves (A and B) shifted by 90°, enabling **direction detection**.  
- Push-button press is detected on a separate **SW** pin.

### 2.2 Pin Descriptions
<p float="left" align="center">
  <img src="https://github.com/user-attachments/assets/3ab9a5b2-b1c5-4e0f-b5b1-0c987b105e9a" width="300" height="220" />
  <img src="https://github.com/user-attachments/assets/d082e633-0fe7-488d-91b1-ffaae9e563cf" width="300" height="220" />
</p>


* Rotary encoder outputs **LOW** when its internal switches close.
* LOW is created by grounding pin **C**, which connects to **CLK** and **DT** when switches close.
* When switches are open, **CLK** and **DT** go **HIGH** through pull-up resistors powered by **5V**.
* The encoder also includes a **push-button switch** (activated by pressing the shaft).
* The push-button is **normally open** and closes when pressed.
* This feature can be used for mode switching, e.g., **coarse vs. fine adjustment**.

---
## 3. Rotary Encoder Integration With NodeMCU v1

This section explains how to connect the rotary encoder to the NodeMCU, configure the Arduino development environment, upload the firmware, and perform basic testing and data acquisition.

---

## 3.1 Hardware Connections

### Wiring

```
Rotary Encoder               NodeMCU v1

VCC -----------------------> 3V3
GND -----------------------> GND
A (CLK) -------------------> D1  (GPIO5)
B (DT) -------------------> D2  (GPIO4)
SW ------------------------> D5  (GPIO14)
```

### Notes

* Configure CLK, DT, and SW pins as **INPUT_PULLUP**.
* NodeMCU uses **3.3V logic** → Do **not** power encoder with 5V.
* Keep wiring short to reduce switch bounce and noise.

---

## 3.2 Arduino IDE Setup

Follow these steps to prepare your development environment:

### **Step 1 — Install Arduino IDE**

Download from: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

### **Step 2 — Add ESP8266 Board Package**

1. Open **File → Preferences**
2. In *Additional Board Manager URLs*, add:

   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools → Board → Boards Manager**
4. Search for **ESP8266** and install.

### **Step 3 — Select NodeMCU v1**

Go to:
**Tools → Board → ESP32 Arduino → ESP32 Dev Module**


### **Step 4 — Select COM Port**

Go to:
**Tools → Port → /dev/ttyUSB0 (Linux)”**

---

## 3.3 Firmware Example

### Minimal Rotary Encoder Reading Code

```cpp
// Rotary Encoder Test for ESP32
// Module: ESP32 Dev Module
// Wiring:
// CLK -> GPIO14
// DT  -> GPIO27
// SW  -> GPIO26 (button)

const int pinA = 14;    // CLK
const int pinB = 27;    // DT
const int pinSW = 26;   // Button

volatile int encoderPos = 0;
volatile int lastEncoded = 0;

// Interrupt function to decode encoder rotation
void IRAM_ATTR updateEncoder() {
  int MSB = digitalRead(pinA);
  int LSB = digitalRead(pinB);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPos++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPos--;

  lastEncoded = encoded;
}

void setup() {
  Serial.begin(115200);

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinSW, INPUT_PULLUP);

  // Attach interrupts to both encoder channels
  attachInterrupt(digitalPinToInterrupt(pinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB), updateEncoder, CHANGE);

  Serial.println("Rotary Encoder Test Ready");
}

void loop() {
  static int lastPos = 0;

  // Print encoder position if changed
  if (encoderPos != lastPos) {
    Serial.print("Position: ");
    Serial.println(encoderPos);
    lastPos = encoderPos;
  }

  // Detect button press
  if (digitalRead(pinSW) == LOW) {
    Serial.println("Button pressed!");
    delay(300); // simple debounce
  }

  delay(10);
}

```

### Uploading the Code

Click the **Upload** button → wait for "Done Uploading".

---

## 3.4 Observing & Acquiring Data

### Observe Live Data

1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Rotate encoder → observe `Position: <value>`
4. Press encoder button → see `Button Pressed`

---

## 3.5 Using the *RotaryServoBlock* Library (RotaryServoBlock.h, RotaryServoBlock.cpp)

The **RotaryServoBlock** library simplifies reading a rotary encoder and controlling a servo on the ESP32.
It automatically handles quadrature decoding, angle clamping, servo movement, and button debouncing.

### **Library Functional Capabilities**

* **Maps rotary encoder rotation to a servo angle (0–180° by default)**

  * Full encoder rotation = full servo sweep
  * Adjustable `maxAngle` (0–180 or any limit you choose)

* **Clamps angle to safe limits**

  * Never goes below **0°**
  * Never exceeds **maxAngle**
  * Prevents servo damage

* **Supports 20-pulse (80-step) rotary encoder**

  * Uses full 4× decoding for smooth, precise motion

* **Tracks unlimited encoder rotation (multi-turn)**

  * Servo stays within its valid range
  * Angle moves immediately when rotating back from a limit

* **Drives hobby servos using ESP32 LEDC PWM**

  * Smooth, stable signal
  * Compatible with standard 50 Hz servos

* **Push-button support with debounce**

  * Detects short press
  * Allows user-defined callback function

* **Non-blocking operation**

  * Entire library runs without delays
  * Does not interfere with other tasks

---


### Basic Example

```cpp
#include "RotaryServoBlock.h"

RotaryServoBlock rotary(14, 27, 26, 25);  // pinA, pinB, pinSW, servoPin

void setup() {
  Serial.begin(115200);
  rotary.begin(80, 180);                 // 80 steps/rev, 180° range
  rotary.onButton([](){
    Serial.println("Button pressed!");
  });
}

void loop() {
  rotary.update();                       // handle button
  rotary.setAngle(rotary.getAngle());    // move servo to computed angle
}
```

### Expected Output (Serial Monitor)

```
Angle: 12°
Angle: 16°
Angle: 20°
...
Button pressed!
```


---



### Save Encoder Data

In Serial Monitor:

* Click **“Save Output”**
* Save as **CSV** or **TXT** file

Recommended formats:

* **CSV (preferred):** `timestamp,position`
* **TXT:** raw logs

Example CSV entry:

```
0.120, 5
0.130, 6
0.140, 7
```

---

## 3.5 Test Procedures

### Test Table

| Test Case | Action                  | Expected Output                                 |
| --------- | ----------------------- | ----------------------------------------------- |
| 1         | Rotate clockwise        | Position increases (+1 per detent)              |
| 2         | Rotate counterclockwise | Position decreases (–1 per detent)              |
| 3         | Press SW button         | “Button Pressed” printed                        |
| 4         | Fast rotation           | Position changes smoothly with no missed pulses |
| 5         | Idle (no movement)      | No output except button events                  |

---