#include <RCSwitch.h>

RCSwitch rf;

void setup() {
  Serial.begin(9600);
  rf.enableReceive(0);
  rf.setProtocol(5);
  Serial.println("OK:INICIADO - presiona el mando RF");
}

void loop() {
  if (rf.available()) {
    unsigned long valor = rf.getReceivedValue();
    unsigned int bits = rf.getReceivedBitlength();
    unsigned int proto = rf.getReceivedProtocol();
    rf.resetAvailable();
    if (valor != 0 && bits > 0) {
      Serial.print("RF:");
      Serial.print(valor);
      Serial.print(" BITS:");
      Serial.print(bits);
      Serial.print(" PROTO:");
      Serial.println(proto);
    }
  }
}
