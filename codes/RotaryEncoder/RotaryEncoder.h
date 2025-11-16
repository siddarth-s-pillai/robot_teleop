#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

struct RotaryData {
    long steps;
    int direction;
    bool button;
};

class RotaryEncoder {
public:
    RotaryEncoder(int pinA, int pinB, int pinSW);

    void begin();
    void update();
    long getSteps();
    int getDirection();
    bool getButton();

    RotaryData getRotaryEncoderData();   // NEW

private:
    int _pinA, _pinB, _pinSW;
    long _steps;
    int _direction;
    int _lastCLK;
    unsigned long _lastButton;
};

#endif
