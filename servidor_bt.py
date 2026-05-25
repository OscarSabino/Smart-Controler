#!/usr/bin/env python
# -*- coding: utf-8 -*-

# =============================================
# Servidor Bluetooth para Smart Controller
# Recibe comandos desde la web y los envia
# al Arduino por Bluetooth (Serial)
# =============================================

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import os
import sys
import time

# =============================================
# CONFIGURACION
# =============================================

PUERTO = 5000
BT_COM = os.environ.get("BT_COM", "COM4")
BT_BAUD = 9600
TIMEOUT_ESCANEO = 30  # segundos esperando codigo del Arduino

# Variable global para el puerto serie
bt = None


# =============================================
# MANEJADOR DE PETICIONES HTTP
# =============================================

class Manejador(BaseHTTPRequestHandler):

    def do_OPTIONS(self):
        self.enviar_headers(200)

    def do_GET(self):
        if self.path == "/":
            self.enviar_json({"mensaje": "Servidor Bluetooth funcionando", "puerto": BT_COM})
        else:
            self.enviar_error(404, "Ruta no encontrada")

    def do_POST(self):
        if self.path == "/enviar":
            self.recibir_comando()
        elif self.path == "/escanear":
            self.escanear_codigo()
        else:
            self.enviar_error(404, "Ruta no encontrada")

    # =============================================
    # ENVIAR UN COMANDO AL ARDUINO
    # =============================================
    def recibir_comando(self):
        try:
            datos = self.leer_json()
            protocolo = datos.get("protocolo", "IR")
            codigo = datos.get("codigo", "")

            if not codigo:
                self.enviar_error(400, "Falta el codigo del comando")
                return

            trama = protocolo + ":" + codigo + "\n"
            print("Enviando: " + trama.strip())

            if enviar_por_bluetooth(trama):
                self.enviar_json({"mensaje": "Señal enviada correctamente", "protocolo": protocolo, "codigo": codigo})
            else:
                self.enviar_error(500, "No se pudo enviar. El Arduino no esta conectado en " + BT_COM)

        except Exception as e:
            print("Error: " + str(e))
            self.enviar_error(500, "Error interno: " + str(e))

    # =============================================
    # ESCANEAR UN CODIGO DESDE EL ARDUINO
    # =============================================
    def escanear_codigo(self):
        global bt

        try:
            datos = self.leer_json()
            protocolo = datos.get("protocolo", "IR")
        except:
            protocolo = "IR"

        # Abrimos el puerto serie si no esta abierto
        if not bt or not bt.is_open:
            try:
                import serial
                bt = serial.Serial(BT_COM, BT_BAUD, timeout=1)
            except Exception as e:
                self.enviar_error(500, "No se puede abrir " + BT_COM + ": " + str(e))
                return

        # Enviamos SCAN al Arduino
        print("Enviando SCAN al Arduino...")
        bt.write(b"SCAN\n")
        time.sleep(0.5)

        # Esperamos respuesta con timeout
        inicio = time.time()
        respuesta = ""
        while time.time() - inicio < TIMEOUT_ESCANEO:
            if bt.in_waiting > 0:
                linea = bt.readline().decode("utf-8").strip()
                print("Arduino dice: " + linea)
                respuesta = linea

                # Si el Arduino envia OK con el codigo
                if respuesta.startswith("OK:CODIGO:"):
                    codigo = respuesta[10:]  # Quitamos "OK:CODIGO:"
                    self.enviar_json({"codigo": codigo, "protocolo": protocolo})
                    return
                elif respuesta.startswith("ERROR"):
                    self.enviar_error(400, "Error del Arduino: " + respuesta)
                    return
            else:
                time.sleep(0.1)

        # Timeout
        self.enviar_error(408, "Tiempo de espera agotado. No se recibio ningun codigo")

    # =============================================
    # FUNCIONES AYUDANTES
    # =============================================

    def leer_json(self):
        contenido = self.rfile.read(int(self.headers["Content-Length"]))
        return json.loads(contenido)

    def enviar_headers(self, codigo):
        self.send_response(codigo)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def enviar_json(self, datos):
        self.enviar_headers(200)
        self.wfile.write(json.dumps(datos).encode("utf-8"))

    def enviar_error(self, codigo, mensaje):
        self.enviar_headers(codigo)
        self.wfile.write(json.dumps({"error": mensaje}).encode("utf-8"))

    def log_message(self, formato, *args):
        print(formato % args)


# =============================================
# ENVIAR COMANDO AL ARDUINO POR BLUETOOTH
# =============================================

def enviar_por_bluetooth(trama):
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial no esta instalado. pip install pyserial")
        return False

    try:
        bt_temp = serial.Serial(BT_COM, BT_BAUD, timeout=2)
        bt_temp.write(trama.encode("utf-8"))
        bt_temp.close()
        print("OK: Comando enviado por " + BT_COM)
        return True
    except Exception as e:
        print("ERROR al enviar por Bluetooth: " + str(e))
        return False


# =============================================
# INICIAR EL SERVIDOR
# =============================================

if __name__ == "__main__":
    print("====================================")
    print("  SMART CONTROLLER - Servidor BT")
    print("====================================")
    print("")
    print("Escuchando en http://localhost:" + str(PUERTO))
    print("Bluetooth en " + BT_COM)
    print("")
    print("Endpoints:")
    print("  GET  /          - Estado del servidor")
    print("  POST /enviar    - Enviar comando al Arduino")
    print("  POST /escanear  - Escanear codigo desde el Arduino")
    print("")
    print("Presiona Ctrl+C para salir")
    print("")

    servidor = HTTPServer(("0.0.0.0", PUERTO), Manejador)

    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("")
        print("Servidor detenido")
        if bt and bt.is_open:
            bt.close()
        servidor.server_close()
