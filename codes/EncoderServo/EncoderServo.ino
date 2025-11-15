#include "RotaryServoBlock.h"

RotaryServoBlock rotary(14, 27, 26, 25);  // CLK, DT, SW, Servo

void setup() {
    Serial.begin(115200);

    // 80 steps per revolution (20 pulses x4)
    rotary.begin(80, 180);

    rotary.onButton([](){
        Serial.println("Button pressed!");
    });
}

void loop() {
    rotary.update();
    float angle = rotary.getAngle();
    rotary.setAngle(angle);
    delay(100);
}
