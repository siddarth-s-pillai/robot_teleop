/*
  RotaryServoBlock - ESP32 Rotary Encoder to Servo Controller Library
  -------------------------------------------------------------------
  Version: 1.1
  Author: Your Name
  Platform: ESP32 (Arduino Core 2.x / 3.x)

  Description:
    Control a servo using a rotary encoder and optional push-button.
    Designed for encoders with 20 pulses per revolution (80 steps with 4× decoding),
    but works with any quadrature rotary encoder by configuring stepsPerRev.

    Features:
      - Angle is clamped between 0° and maxAngle (default 180°).
      - Saturating behavior: turning past 0°/maxAngle keeps angle at limits.
      - **Immediate reverse recovery**: rotating opposite direction leaves the limit instantly.
      - Real-time Serial angle output (prints only on change).
      - Optional push-button with debounce and callback support.
      - Multi-turn encoder support (360° continuous rotation).
      - Designed for smooth servo motion using ESP32 LEDC PWM.

  Usage:
      RotaryServoBlock rotary(pinA, pinB, pinSW, servoPin);

      rotary.begin(stepsPerRev, maxAngle);
      rotary.onButton(callbackFunction);   // optional
      rotary.update();                     // call in loop()
      float angle = rotary.getAngle();     // get clamped angle
      rotary.setAngle(angle);              // move servo

  Wiring:
      Rotary Encoder:
        CLK (A)  -> pinA
        DT  (B)  -> pinB
        SW  (Button) -> pinSW

      Servo:
        Signal -> servoPin
        VCC    -> 5V
        GND    -> ESP32 GND

  Notes:
      - Default stepsPerRev = 80 (typical 20-pulse encoder with 4× decoding)
      - One full encoder turn maps to full servo sweep (0° → maxAngle)
      - Safe interrupt-driven quadrature decoding
*/

#ifndef ROTARY_SERVO_BLOCK_H
#define ROTARY_SERVO_BLOCK_H

#include <Arduino.h>

class RotaryServoBlock {
public:
    // Constructor: pinA = CLK, pinB = DT, pinSW = Button, servoPin = Servo signal pin
    RotaryServoBlock(int pinA, int pinB, int pinSW, int servoPin);

    // Initialize encoder & servo
    // stepsPerRevolution: encoder steps per turn (80 for typical 20-pulse encoder)
    // maxAngle: maximum servo angle (default 180°)
    void begin(int stepsPerRevolution = 80, float maxAngle = 180.0f);

    // Get current angle (always clamped 0..maxAngle)
    float getAngle();

    // Get raw encoder count (0.._encoderMax)
    long getRawCount();

    // Move servo to a specific angle (clamped)
    void setAngle(float angle);

    // Register callback for button press
    void onButton(void (*callback)());

    // Call frequently in loop() to handle button input
    void update();

private:
    int _pinA, _pinB, _pinSW, _servoPin;
    int _stepsPerRev;
    float _maxAngle;

    volatile long _encoderPos;   // internal encoder counter (ISR)
    long _encoderMax;            // max valid encoder count (maps to maxAngle)

    int _lastEncoded;
    unsigned long _lastButton;
    float _lastAngle;

    void (*_buttonCallback)() = nullptr;

    static RotaryServoBlock* instance;

    // Quadrature decoding ISR (interrupt-driven)
    static void IRAM_ATTR updateISR();
};

#endif
