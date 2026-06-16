/*
  SMART CONTROLLER - Arduino
  USB (Serial) + Bluetooth (HC-05)
  IR (receiver + sender) + RF 433MHz (RH_ASK)

  COMANDOS:
    IR:CODIGOHEX  -> Enviar codigo IR especifico
    RF:CODIGO     -> Enviar codigo RF especifico
    SEND_IR       -> Re-enviar ultimo IR capturado
    SEND_RF       -> Re-enviar ultimo RF capturado
    TEST          -> Responder OK:TEST
    SCAN          -> Limpiar buffer y entrar en modo scan

  SALIDA (por IR/RF capturado):
    IR:CODIGOHEX
    RF:TEXTO
    OK:... | ERROR:...
*/

#define IR_RECEIVE_PIN  8
#define IR_SEND_PIN     9
#define PIN_RF_SEND     12
#define PIN_RF_RECV     11
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
#include <RH_ASK.h>
#include <SPI.h>
#include <SoftwareSerial.h>

RH_ASK rf_driver(2000, PIN_RF_RECV, PIN_RF_SEND);
SoftwareSerial BT(BT_RX, BT_TX);

unsigned long ultimoIR = 0;
String ultimoRF = "";
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
  rf_driver.init();

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

void enviarRF(String codigo) {
  rf_driver.send((uint8_t*)codigo.c_str(), codigo.length());
  rf_driver.waitPacketSent();
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
    if (ultimoRF == "") {
      enviar("ERROR:No hay codigo RF capturado");
    } else {
      enviarRF(ultimoRF);
    }
  }
  else if (cmd.startsWith("SCAN")) {
    ultimoIR = 0;
    ultimoRF = "";
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
    if (val == "") {
      enviar("ERROR:Codigo RF invalido");
    } else {
      enviarRF(val);
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

  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if (modoScan && rf_driver.recv(buf, &buflen)) {
    if (buflen > 0) {
      buf[buflen] = '\0';
      String msg = String((char*)buf);
      msg.trim();
      if (msg != "") {
        ultimoRF = msg;
        enviar("RF:" + msg);
        modoScan = false;
      }
    }
  }

  if (modoScan && millis() - scanInicio > SCAN_TIMEOUT) {
    modoScan = false;
  }

  leerComandos();
}
