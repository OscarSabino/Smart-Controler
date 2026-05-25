/*
 * Smart Controller para Arduino
 * Recibe comandos por Bluetooth y controla IR/RF
 */

#include <IRremote.h>

// Pines
int PIN_IR = 3;    // LED infrarrojo
int PIN_RF = 4;    // Modulo RF

// Receptor IR para aprender codigos
IRrecv irrecv(5);

// Modo escaneo
bool escaneando = false;
unsigned long inicioScan = 0;

void setup() {
    Serial.begin(9600);

    pinMode(PIN_RF, OUTPUT);
    digitalWrite(PIN_RF, LOW);

    IrSender.begin(PIN_IR);

    Serial.println("Listo");
}

void loop() {
    // Modo escaneo: esperamos una señal IR
    if (escaneando) {
        if (IrReceiver.decode()) {
            unsigned long valor = IrReceiver.decodedIRData.decodedRawData;
            if (valor == 0) {
                valor = IrReceiver.decodedIRData.command;
            }

            Serial.print("OK:CODIGO:");
            Serial.print("NEC:0x");
            Serial.println(valor, HEX);

            IrReceiver.resume();
            escaneando = false;
            irrecv.disableIRIn();
            return;
        }

        // Timeout 30 segundos
        if (millis() - inicioScan > 30000) {
            Serial.println("ERROR: Tiempo agotado");
            escaneando = false;
            irrecv.disableIRIn();
            return;
        }
    }

    // Leer comandos del Bluetooth
    if (Serial.available() > 0) {
        String trama = Serial.readStringUntil('\n');
        trama.trim();

        if (trama == "SCAN") {
            escaneando = true;
            inicioScan = millis();
            irrecv.enableIRIn();
            Serial.println("SCAN: Esperando...");
        } else if (trama.length() > 0) {
            // Buscamos los dos puntos
            int pos = trama.indexOf(':');
            if (pos > 0) {
                String proto = trama.substring(0, pos);
                String codigo = trama.substring(pos + 1);

                if (proto == "IR") {
                    int sep = codigo.indexOf(':');
                    if (sep > 0) {
                        String hex = codigo.substring(sep + 1);
                        long val = strtol(hex.c_str(), NULL, 16);
                        IrSender.sendNEC(val, 32);
                        Serial.println("OK: IR enviado");
                    }
                } else if (proto == "RF") {
                    for (int i = 0; i < 10; i++) {
                        digitalWrite(PIN_RF, HIGH);
                        delay(1);
                        digitalWrite(PIN_RF, LOW);
                        delay(1);
                    }
                    Serial.println("OK: RF enviado");
                }
            }
        }
    }
}
