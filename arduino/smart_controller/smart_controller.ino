/*
  SMART CONTROLLER - Arduino
  USB (Serial) + Bluetooth (HC-05)

  COMANDOS:
    IR:CODIGOHEX  -> Enviar codigo IR especifico
    RF:CODIGO     -> Enviar codigo RF especifico
    SEND_IR       -> Re-enviar ultimo IR capturado
    SEND_RF       -> Re-enviar ultimo RF capturado
    TEST          -> Responder OK:TEST
    SCAN          -> Limpiar buffer y entrar en modo scan

  SALIDA (por IR/RF capturado):
    IR:CODIGOHEX
    RF:CODIGO
    OK:... | ERROR:...
*/

#define IR_RECEIVE_PIN  2
#define IR_SEND_PIN     3
#define PIN_RF_SEND     4
#define PIN_RF_RECV     5
#define BT_RX          10
#define BT_TX          11

// Habilitar los decodificadores de protocolos IR mas comunes
#define DECODE_NEC
#define DECODE_SONY
#define DECODE_RC5
#define DECODE_RC6
#define DECODE_SAMSUNG
#define DECODE_LG
#define DECODE_JVC
// NO habilitar DECODE_HASH para evitar codigos falsos por ruido

#include <IRremote.hpp>
#include <RCSwitch.h>
#include <SoftwareSerial.h>

RCSwitch rfSwitch = RCSwitch();
SoftwareSerial BT(BT_RX, BT_TX);

unsigned long ultimoIR = 0;
unsigned long ultimoRF = 0;
bool modoScan = false;
unsigned long scanInicio = 0;
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
  rfSwitch.enableTransmit(PIN_RF_SEND);
  rfSwitch.enableReceive(PIN_RF_RECV);

  delay(1000);
  Serial.flush();
  while (Serial.available()) Serial.read();
  while (BT.available()) BT.read();

  enviar("OK:INICIADO");
}

void enviarIR(unsigned long codigo) {
  IrSender.sendNEC(codigo, 32);
  ultimoIR = codigo;
  enviar("OK:IR enviado");
}

void enviarRF(unsigned long codigo) {
  rfSwitch.send(codigo, 24);
  ultimoRF = codigo;
  enviar("OK:RF enviado");
}

void procesar(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "TEST") {
    enviar("OK:TEST");
  }
  else if (cmd == "SEND_IR") {
    if (ultimoIR == 0) {
      enviar("ERROR:No hay codigo IR capturado");
    } else {
      enviarIR(ultimoIR);
    }
  }
  else if (cmd == "SEND_RF") {
    if (ultimoRF == 0) {
      enviar("ERROR:No hay codigo RF capturado");
    } else {
      enviarRF(ultimoRF);
    }
  }
  else if (cmd.startsWith("SCAN")) {
    ultimoIR = 0;
    ultimoRF = 0;
    modoScan = true;
    scanInicio = millis();
    enviar("OK:SCAN_LISTENING");
  }
  else if (cmd.startsWith("IR:")) {
    unsigned long codigo = strtoul(cmd.substring(3).c_str(), NULL, 16);
    if (codigo == 0 || codigo == 0xFFFFFFFF) {
      enviar("ERROR:Codigo IR invalido");
    } else {
      enviarIR(codigo);
    }
  }
  else if (cmd.startsWith("RF:")) {
    String val = cmd.substring(3);
    val.trim();
    unsigned long codigo;
    if (val.startsWith("0x") || val.startsWith("0X")) {
      codigo = strtoul(val.c_str(), NULL, 16);
    } else {
      codigo = strtoul(val.c_str(), NULL, 10);
    }
    if (codigo == 0) {
      enviar("ERROR:Codigo RF invalido");
    } else {
      enviarRF(codigo);
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
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) && modoScan) {
      // Solo aceptar si un decodificador reconocio el protocolo (NEC, Sony, etc.)
      if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
        unsigned long valor = IrReceiver.decodedIRData.decodedRawData;
        if (valor != 0 && valor != 0xFFFFFFFF) {
          ultimoIR = valor;
          enviar("IR:" + String(ultimoIR, HEX));
          modoScan = false;
        }
      }
    }
    IrReceiver.resume();
  }

  if (rfSwitch.available()) {
    if (rfSwitch.getReceivedValue() != 0 && modoScan) {
      ultimoRF = rfSwitch.getReceivedValue();
      enviar("RF:" + String(ultimoRF));
      modoScan = false;
    }
    rfSwitch.resetAvailable();
  }

  if (modoScan && millis() - scanInicio > SCAN_TIMEOUT) {
    modoScan = false;
    enviar("ERROR:SCAN_TIMEOUT");
  }

  leerComandos();
}
