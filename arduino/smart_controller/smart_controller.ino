/*
  SMART CONTROLLER - Arduino
  USB (Serial) + Bluetooth (HC-05)
  IR (receiver + sender) + RF 433MHz (RCSwitch)
*/

#define IR_RECEIVE_PIN  8
#define IR_SEND_PIN     9
#define PIN_RCSWITCH_RX 2
#define PIN_RCSWITCH_TX 4
#define BT_RX          6
#define BT_TX          7

#define DECODE_NEC
#define DECODE_SONY
#define DECODE_RC5
#define DECODE_RC6
#define DECODE_SAMSUNG
#define DECODE_LG
#define DECODE_JVC

#include <IRremote.hpp>
#include <RCSwitch.h>
#include <SoftwareSerial.h>

RCSwitch rf;
SoftwareSerial BT(BT_RX, BT_TX);

unsigned long ultimoIR = 0;
int ultimoIRProto = NEC;
unsigned int ultimoIRBits = 32;
uint8_t irRawTicks[200];
uint8_t irRawLen = 0;
uint8_t irRawKhz = 38;
unsigned long ultimoRF = 0;
unsigned int ultimoRFBits = 0;
unsigned int ultimoRFProto = 0;
unsigned int ultimoRFDelay = 0;
const unsigned long SCAN_TIMEOUT = 10000;

void enviar(String msg) {
  Serial.println(msg);
  BT.println(msg);
}

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);

  rf.enableReceive(0);
  rf.enableTransmit(PIN_RCSWITCH_TX);
  rf.setReceiveTolerance(90);

  delay(1000);
  Serial.flush();
  while (Serial.available()) Serial.read();
  while (BT.available()) BT.read();

  enviar("OK:INICIADO");
}

void enviarIR(unsigned long codigo, int proto = NEC, unsigned int bits = 32) {
  if (irRawLen > 0 && ultimoIR == codigo && ultimoIRProto == proto && ultimoIRBits == bits) {
    IrSender.sendRaw(irRawTicks, irRawLen, irRawKhz);
    delay(100);
    IrSender.sendRaw(irRawTicks, irRawLen, irRawKhz);
  } else {
    switch (proto) {
      case NEC:
        IrSender.sendNECRaw(codigo, 3);
        break;
      case SONY:
        IrSender.sendSonyMSB(codigo, bits ? bits : 12);
        break;
      case RC5:
        IrSender.sendRC5(codigo, bits ? bits : 12);
        break;
      case RC6:
        IrSender.sendRC6((uint32_t)codigo, bits ? bits : 20);
        break;
      case SAMSUNG:
        IrSender.sendSamsungMSB(codigo, bits ? bits : 32);
        break;
      case LG:
        IrSender.sendLG(codigo, bits ? bits : 28);
        break;
      case JVC:
        IrSender.sendJVCMSB(codigo, bits ? bits : 16, false);
        break;
      default:
        IrSender.sendNECRaw(codigo, 3);
        break;
    }
    irRawLen = 0;
  }
  ultimoIR = codigo;
  ultimoIRProto = proto;
  ultimoIRBits = bits;
  enviar("OK:IR enviado");
}

void enviarRF(unsigned long codigo, unsigned int bits, unsigned int proto, unsigned int pulso) {
  if (proto) rf.setProtocol(proto);
  if (pulso) rf.setPulseLength(pulso);
  rf.send(codigo, bits ? bits : 24);
  ultimoRF = codigo;
  ultimoRFBits = bits;
  ultimoRFProto = proto;
  ultimoRFDelay = pulso;
  enviar("OK:RF enviado");
}

void procesar(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "TEST") {
    enviar("OK:TEST");
  }
  else if (cmd == "TEST_IR") {
    IrSender.sendNECRaw(0x00FFA25D, 3);
    enviar("OK:TEST_IR enviado (0xFFA25D x4)");
  }
  else if (cmd == "TEST_IR_MSB") {
    IrSender.sendNECMSB(0x00FFA25D, 32, false);
    enviar("OK:TEST_IR_MSB enviado");
  }
  else if (cmd == "SEND_IR") {
    if (ultimoIR == 0) {
      enviar("ERROR:No hay codigo IR capturado");
    } else {
      enviarIR(ultimoIR, ultimoIRProto, ultimoIRBits);
    }
  }
  else if (cmd == "SEND_RF") {
    if (ultimoRF == 0) {
      enviar("ERROR:No hay codigo RF capturado");
    } else {
      enviarRF(ultimoRF, ultimoRFBits, ultimoRFProto, ultimoRFDelay);
    }
  }
  else if (cmd.startsWith("SCAN")) {
    ultimoIR = 0;
    ultimoRF = 0;
    irRawLen = 0;

    Serial.println("OK:SCAN_LISTENING");
    Serial.println("DEBUG:SCAN_INICIADO");

    unsigned long inicio = millis();
    unsigned long lastDebug = 0;

    while (true) {
      if (rf.available()) {
        unsigned long valor = rf.getReceivedValue();
        unsigned int bits = rf.getReceivedBitlength();
        unsigned int proto = rf.getReceivedProtocol();
        unsigned int d = rf.getReceivedDelay();
        rf.resetAvailable();

        if (valor != 0 && bits > 0) {
          ultimoRF = valor;
          ultimoRFBits = bits;
          ultimoRFProto = proto;
          ultimoRFDelay = d;
          Serial.print("RF:");
          Serial.print(ultimoRF);
          Serial.print(",");
          Serial.print(ultimoRFBits);
          Serial.print(",");
          Serial.print(ultimoRFProto);
          Serial.print(",");
          Serial.println(ultimoRFDelay);
        }
      }

      if (IrReceiver.decode()) {
        if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
          if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
            unsigned long valor = IrReceiver.decodedIRData.decodedRawData;
            if (valor != 0 && valor != 0xFFFFFFFF) {
              ultimoIR = valor;
              ultimoIRProto = IrReceiver.decodedIRData.protocol;
              ultimoIRBits = IrReceiver.decodedIRData.numberOfBits;

              irRawLen = IrReceiver.decodedIRData.rawlen - 1;
              for (uint8_t i = 0; i < irRawLen && i < 200; i++) {
                irRawTicks[i] = IrReceiver.irparams.rawbuf[i + 1];
              }
              switch (ultimoIRProto) {
                case NEC: case SAMSUNG: case LG: case JVC:
                  irRawKhz = 38; break;
                case SONY:
                  irRawKhz = 40; break;
                case RC5: case RC6:
                  irRawKhz = 36; break;
                default:
                  irRawKhz = 38; break;
              }

              Serial.print("IR:");
              Serial.print(ultimoIR, HEX);
              Serial.print(",");
              Serial.print(ultimoIRProto);
              Serial.print(",");
              Serial.println(ultimoIRBits);
            }
          }
        }
        IrReceiver.resume();
      }

      if (millis() - inicio > SCAN_TIMEOUT) {
        Serial.println("DEBUG:SCAN_TIMEOUT");
        break;
      }

      if (millis() - lastDebug > 3000) {
        lastDebug = millis();
        Serial.println("DEBUG:ESPERANDO_RF...");
      }
    }
  }
  else if (cmd.startsWith("IR:")) {
    String resto = cmd.substring(3);
    unsigned long codigo;
    int proto = ultimoIRProto;
    unsigned int bits = ultimoIRBits;

    int c1 = resto.indexOf(',');
    if (c1 > 0) {
      codigo = strtoul(resto.substring(0, c1).c_str(), NULL, 16);
      int c2 = resto.indexOf(',', c1 + 1);
      if (c2 > 0) {
        proto = resto.substring(c1 + 1, c2).toInt();
        bits = (unsigned int)resto.substring(c2 + 1).toInt();
      }
    } else {
      codigo = strtoul(resto.c_str(), NULL, 16);
    }

    if (codigo == 0 || codigo == 0xFFFFFFFF) {
      enviar("ERROR:Codigo IR invalido");
    } else {
      enviarIR(codigo, proto, bits);
    }
  }
  else if (cmd.startsWith("RF:")) {
    unsigned long codigo = strtoul(cmd.substring(3).c_str(), NULL, 10);
    if (codigo == 0) {
      enviar("ERROR:Codigo RF invalido");
    } else {
      enviarRF(codigo, ultimoRFBits, ultimoRFProto, ultimoRFDelay);
    }
  }
}

String cmdSerial = "";
String cmdBT = "";

void leerComandos() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (cmdSerial.length() > 0) { procesar(cmdSerial); cmdSerial = ""; }
    } else if (c != '\r') {
      cmdSerial += c;
    }
  }
  while (BT.available()) {
    char c = BT.read();
    if (c == '\n') {
      if (cmdBT.length() > 0) { procesar(cmdBT); cmdBT = ""; }
    } else if (c != '\r') {
      cmdBT += c;
    }
  }
}

void loop() {
  if (IrReceiver.decode()) {
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
        unsigned long valor = IrReceiver.decodedIRData.decodedRawData;
        if (valor != 0 && valor != 0xFFFFFFFF) {
          ultimoIR = valor;
          ultimoIRProto = IrReceiver.decodedIRData.protocol;
          ultimoIRBits = IrReceiver.decodedIRData.numberOfBits;

          irRawLen = IrReceiver.decodedIRData.rawlen - 1;
          for (uint8_t i = 0; i < irRawLen && i < 200; i++) {
            irRawTicks[i] = IrReceiver.irparams.rawbuf[i + 1];
          }
          switch (ultimoIRProto) {
            case NEC: case SAMSUNG: case LG: case JVC:
              irRawKhz = 38; break;
            case SONY:
              irRawKhz = 40; break;
            case RC5: case RC6:
              irRawKhz = 36; break;
            default:
              irRawKhz = 38; break;
          }

          enviar("IR:" + String(ultimoIR, HEX) + "," + ultimoIRProto + "," + ultimoIRBits);
        }
      }
    }
    IrReceiver.resume();
  }

  leerComandos();
}
