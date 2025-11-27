#include "RotaryEncoder.hpp"

RotaryEncoder::RotaryEncoder(int pinA, int pinB, int pinSW)
: _pinA(pinA), _pinB(pinB), _pinSW(pinSW),
  _steps(0), _direction(0), _lastCLK(0), _lastButton(0)
{
}

void RotaryEncoder::begin() {
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinSW, INPUT_PULLUP);

    _lastCLK = digitalRead(_pinA);
}

void RotaryEncoder::update() {
    int clk = digitalRead(_pinA);

    // Detect movement
    if (clk != _lastCLK) {
        int dt = digitalRead(_pinB);

        if (dt != clk) {
            _steps++;
            _direction = 1;
        } else {
            _steps--;
            _direction = -1;
        }
    }

    _lastCLK = clk;

    // Simple button debounce
    if (digitalRead(_pinSW) == LOW) {
        unsigned long now = millis();
        if (now - _lastButton > 20) {
            _lastButton = now;
        }
    }
}

long RotaryEncoder::getSteps() {
    return _steps;
}

int RotaryEncoder::getDirection() {
    int d = _direction;
    _direction = 0;  // reset after read
    return d;
}

bool RotaryEncoder::getButton() {
    return digitalRead(_pinSW) == LOW;
}

RotaryData RotaryEncoder::getRotaryEncoderData() {
    RotaryData d;
    d.steps = getSteps();
    d.direction = getDirection();
    d.button = getButton();
    return d;
}
