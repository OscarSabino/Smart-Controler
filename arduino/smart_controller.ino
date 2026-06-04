/*
  ---------------------------------------------------------
  SMART CONTROLLER – Código Arduino (Versión Estudiante)
  Autor: Óscar
  Descripción:
  Este programa permite:
    - Capturar señales IR (infrarrojas)
    - Reproducir señales IR
    - Capturar señales RF (433 MHz)
    - Reproducir señales RF
    - Recibir comandos desde Bluetooth HC‑05
  ---------------------------------------------------------
*/

// Librería para señales IR
#include <IRremote.h>

// Librería para señales RF 433 MHz
#include <RCSwitch.h>

// Librería para comunicación Bluetooth por software
#include <SoftwareSerial.h>

// -------------------- CONFIGURACIÓN DE PINES --------------------

// Receptor IR (VS1838B)
#define PIN_IR_RECV 2

// Emisor IR (LED IR + transistor PN2222)
#define PIN_IR_SEND 3

// Emisor RF (FS1000A)
#define PIN_RF_SEND 4

// Receptor RF (XY-MK-5V)
#define PIN_RF_RECV 5

// Bluetooth HC‑05 (SoftwareSerial)
#define BT_RX 10   // HC‑05 TX → Arduino RX
#define BT_TX 11   // HC‑05 RX ← Arduino TX (con divisor de tensión)

// -------------------- OBJETOS DE LIBRERÍAS --------------------

// Objeto para recibir IR
IRrecv irrecv(PIN_IR_RECV);

// Objeto para enviar IR
IRsend irsend(PIN_IR_SEND);

// Estructura donde se guarda el código IR recibido
decode_results results;

// Objeto para RF
RCSwitch rfSwitch = RCSwitch();

// Objeto para Bluetooth
SoftwareSerial BT(BT_RX, BT_TX);

// Variables donde guardamos el último código capturado
unsigned long ultimoIR = 0;
unsigned long ultimoRF = 0;


// -------------------- SETUP --------------------
void setup() {

  // Monitor serie para depuración
  Serial.begin(9600);

  // Bluetooth HC‑05 a 9600 baudios
  BT.begin(9600);

  // Activar receptor IR
  irrecv.enableIRIn();

  // Activar emisor RF
  rfSwitch.enableTransmit(PIN_RF_SEND);

  // Activar receptor RF
  rfSwitch.enableReceive(PIN_RF_RECV);

  // Mensajes de inicio
  Serial.println("Smart Controller iniciado.");
  BT.println("HC-05 conectado. Listo en COM4.");
}


// -------------------- LOOP PRINCIPAL --------------------
void loop() {

  // ---------------------------------------------------------
  // 1. CAPTURA DE SEÑAL IR
  // ---------------------------------------------------------
  if (irrecv.decode(&results)) {

    // Guardamos el código IR recibido
    ultimoIR = results.value;

    // Mostramos por monitor serie
    Serial.print("IR capturado: ");
    Serial.println(ultimoIR, HEX);

    // Enviamos el código por Bluetooth
    BT.print("IR:");
    BT.println(ultimoIR, HEX);

    // Preparamos el receptor para la siguiente señal
    irrecv.resume();
  }


  // ---------------------------------------------------------
  // 2. CAPTURA DE SEÑAL RF
  // ---------------------------------------------------------
  if (rfSwitch.available()) {

    // Guardamos el código RF recibido
    ultimoRF = rfSwitch.getReceivedValue();

    // Mostramos por monitor serie
    Serial.print("RF capturado: ");
    Serial.println(ultimoRF);

    // Enviamos por Bluetooth
    BT.print("RF:");
    BT.println(ultimoRF);

    // Limpiamos el buffer
    rfSwitch.resetAvailable();
  }


  // ---------------------------------------------------------
  // 3. COMANDOS RECIBIDOS POR BLUETOOTH
  // ---------------------------------------------------------
  if (BT.available()) {

    // Leemos el comando enviado desde la app/web
    String cmd = BT.readStringUntil('\n');
    cmd.trim(); // Quitamos espacios y saltos

    // ---- Reproducir IR ----
    if (cmd == "SEND_IR") {
      Serial.println("Enviando IR...");
      irsend.sendNEC(ultimoIR, 32); // NEC es el protocolo más común
    }

    // ---- Reproducir RF ----
    if (cmd == "SEND_RF") {
      Serial.println("Enviando RF...");
      rfSwitch.send(ultimoRF, 24); // 24 bits es lo más habitual
    }

    // ---- Comando de prueba ----
    if (cmd == "TEST") {
      BT.println("OK desde Arduino");
    }
  }
}
