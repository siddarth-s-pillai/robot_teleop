#include "RotaryServoBlock.h"
#include <driver/ledc.h>

RotaryServoBlock* RotaryServoBlock::instance = nullptr;

// Constructor
RotaryServoBlock::RotaryServoBlock(int pinA, int pinB, int pinSW, int servoPin)
: _pinA(pinA), _pinB(pinB), _pinSW(pinSW), _servoPin(servoPin),
  _stepsPerRev(80), _maxAngle(180.0f),
  _encoderPos(0), _encoderMax(80),
  _lastEncoded(0), _lastButton(0), _lastAngle(-1)
{
    instance = this;
}

// ISR — handle encoder rotation with clamping
void IRAM_ATTR RotaryServoBlock::updateISR() {
    int MSB = digitalRead(instance->_pinA);
    int LSB = digitalRead(instance->_pinB);
    int encoded = (MSB << 1) | LSB;
    int sum = (instance->_lastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
        instance->_encoderPos++;
    else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
        instance->_encoderPos--;

    instance->_lastEncoded = encoded;

    // Clamp internal encoder count
    if (instance->_encoderPos > instance->_encoderMax)
        instance->_encoderPos = instance->_encoderMax;
    else if (instance->_encoderPos < 0)
        instance->_encoderPos = 0;
}

// Setup
void RotaryServoBlock::begin(int stepsPerRevolution, float maxAngle) {
    _stepsPerRev = stepsPerRevolution;   // normally 80 for your encoder
    _maxAngle = maxAngle;                // normally 180

    _encoderMax = _stepsPerRev;          // 1 full encoder turn → 180°

    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinSW, INPUT_PULLUP);

    _lastEncoded = (digitalRead(_pinA) << 1) | digitalRead(_pinB);

    attachInterrupt(digitalPinToInterrupt(_pinA), updateISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_pinB), updateISR, CHANGE);

    // Servo PWM setup
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = (gpio_num_t)_servoPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);

    setAngle(getAngle());
}

// Read raw count safely
long RotaryServoBlock::getRawCount() {
    noInterrupts();
    long val = _encoderPos;
    interrupts();
    return val;
}

// Compute angle
float RotaryServoBlock::getAngle() {
    noInterrupts();
    long enc = _encoderPos;
    interrupts();

    float angle = (float)enc / (float)_stepsPerRev * _maxAngle;

    if (angle > _maxAngle) angle = _maxAngle;
    if (angle < 0) angle = 0;

    if (angle != _lastAngle) {
        Serial.print("Angle: ");
        Serial.println(angle, 2);
        _lastAngle = angle;
    }

    return angle;
}

// Move servo
void RotaryServoBlock::setAngle(float angle) {
    if (angle < 0) angle = 0;
    if (angle > _maxAngle) angle = _maxAngle;

    float pulseUs = 500 + (angle / _maxAngle) * 2000;
    uint32_t duty = (pulseUs / 20000.0f) * 65535;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// Button callback
void RotaryServoBlock::onButton(void (*callback)()) {
    _buttonCallback = callback;
}

// Button update
void RotaryServoBlock::update() {
    if (!_buttonCallback) return;

    if (digitalRead(_pinSW) == LOW) {
        unsigned long now = millis();
        if (now - _lastButton > 250) {
            _lastButton = now;
            Serial.println("[BUTTON] Press detected");
            _buttonCallback();
        }
    }
}
