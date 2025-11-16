#include "JsonEncoder.h"

JsonEncoder::JsonEncoder() {}

void JsonEncoder::create(long steps, int dir, bool btn, String &output) {
    doc.clear();
    doc["pos"] = steps;
    doc["dir"] = dir;
    doc["button"] = btn ? 1 : 0;
    doc["time"] = millis();

    serializeJson(doc, output);
}
