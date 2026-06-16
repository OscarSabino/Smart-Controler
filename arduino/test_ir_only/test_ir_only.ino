#include <IRremote.hpp>

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(8, DISABLE_LED_FEEDBACK);
  Serial.println("OK:IR_READY - apunta el mando al pin 8");
}

void loop() {
  if (IrReceiver.decode()) {
    Serial.print("IR:");
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    IrReceiver.resume();
  }
}
