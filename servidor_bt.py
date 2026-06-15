#!/usr/bin/env python
# -*- coding: utf-8 -*-

# =============================================
# Servidor de comunicacion para Smart Controller
# Recibe comandos desde la web y los envia
# al Arduino por USB o Bluetooth (Serial)
# Soporta multiples puertos: USB + BT
# =============================================

from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import json
import os
import time

# =============================================
# CONFIGURACION
# =============================================

PUERTO_HTTP = 5000
PUERTOS_SERIE = [
    os.environ.get("USB_COM", "COM6"),    # USB - Arduino directo
    os.environ.get("BT_COM", "COM4"),     # Bluetooth HC-05
]
BT_BAUD = 9600
TIMEOUT_ESCANEO = 30

# Lista de puertos serie abiertos
puertos_abiertos = []


def abrir_todos_los_puertos():
    global puertos_abiertos
    import serial

    for nombre in PUERTOS_SERIE:
        try:
            p = serial.Serial(nombre, BT_BAUD, timeout=1)
            time.sleep(2)
            puertos_abiertos.append(p)
            print("  [+] " + nombre + " abierto")
        except Exception as e:
            print("  [-] " + nombre + ": " + str(e))


def cerrar_todos_los_puertos():
    global puertos_abiertos
    for p in puertos_abiertos:
        try:
            if p and p.is_open:
                p.close()
        except:
            pass
    puertos_abiertos = []


def puerto_activo():
    """Devuelve el primer puerto disponible"""
    for p in puertos_abiertos:
        try:
            if p and p.is_open:
                return p
        except:
            pass
    return None


def enviar_a_arduino(trama):
    """Envia un comando a TODOS los puertos abiertos"""
    datos = trama.encode()
    for p in puertos_abiertos:
        try:
            if p and p.is_open:
                p.write(datos)
        except:
            pass


def limpiar_buffers():
    """Limpia buffers de entrada de todos los puertos"""
    for p in puertos_abiertos:
        try:
            if p and p.is_open:
                while p.in_waiting > 0:
                    p.readline()
        except:
            pass


# =============================================
# MANEJADOR DE PETICIONES HTTP
# =============================================

class Manejador(BaseHTTPRequestHandler):

    def do_OPTIONS(self):
        self.enviar_headers(200)

    def do_GET(self):
        if self.path == "/":
            estado_puertos = {}
            for nombre in PUERTOS_SERIE:
                encontrado = False
                for p in puertos_abiertos:
                    try:
                        if p and p.is_open and p.port == nombre:
                            estado_puertos[nombre] = "conectado"
                            encontrado = True
                            break
                    except:
                        pass
                if not encontrado:
                    estado_puertos[nombre] = "desconectado"
            self.enviar_json({
                "mensaje": "Servidor funcionando",
                "puertos": estado_puertos,
                "total_conectados": len(puertos_abiertos)
            })
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

            p = puerto_activo()
            if not p:
                self.enviar_error(500, "No hay ningun puerto serie conectado")
                return

            if protocolo.upper() == "RF":
                trama = "RF:" + codigo + "\n"
            else:
                trama = "IR:" + codigo + "\n"

            enviar_a_arduino(trama)
            print("Enviando: " + trama.strip())
            self.enviar_json({
                "mensaje": "Senial enviada correctamente",
                "protocolo": protocolo,
                "codigo": codigo
            })

        except Exception as e:
            print("Error: " + str(e))
            self.enviar_error(500, "Error interno: " + str(e))

    # =============================================
    # ESCANEAR UN CODIGO DESDE EL ARDUINO
    # =============================================
    def escanear_codigo(self):
        try:
            datos = self.leer_json()
            protocolo = datos.get("protocolo", "IR")
        except:
            protocolo = "IR"

        p = puerto_activo()
        if not p:
            self.enviar_error(500, "No hay ningun puerto serie conectado")
            return

        # Limpiamos el buffer de entrada
        time.sleep(0.2)
        try:
            while p.in_waiting > 0:
                p.readline()
        except:
            pass

        # Enviamos SCAN:IR o SCAN:RF al Arduino
        comando = "SCAN:" + protocolo.upper() + "\n"
        try:
            p.write(comando.encode())
        except Exception as e:
            self.enviar_error(500, "Error al escribir en puerto serie: " + str(e))
            return
        print("Modo SCAN activado. Esperando senal " + protocolo + "...")

        inicio = time.time()
        while time.time() - inicio < TIMEOUT_ESCANEO:
            try:
                if p.in_waiting > 0:
                    linea = p.readline().decode("utf-8").strip()
                    print("Arduino dice: " + linea)

                    if linea.startswith(protocolo.upper() + ":"):
                        codigo_hex = linea[3:]
                        codigo_completo = protocolo + ":0x" + codigo_hex
                        self.enviar_json({
                            "codigo": codigo_completo,
                            "protocolo": protocolo
                        })
                        return
                    elif linea.startswith("OK:"):
                        continue
                    elif linea.startswith("ERROR"):
                        if "TIMEOUT" in linea:
                            self.enviar_error(408, "Señal no recibida en el tiempo de espera")
                        else:
                            self.enviar_error(400, "Error del Arduino: " + linea)
                        return
            except Exception as e:
                self.enviar_error(500, "Error leyendo puerto serie: " + str(e))
                return

            time.sleep(0.1)

        self.enviar_error(
            408, "Tiempo de espera agotado. No se recibio ningun codigo"
        )

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
        try:
            self.wfile.write(json.dumps(datos).encode("utf-8"))
        except (ConnectionAbortedError, BrokenPipeError):
            pass

    def enviar_error(self, codigo, mensaje):
        self.enviar_headers(codigo)
        try:
            self.wfile.write(json.dumps({"error": mensaje}).encode("utf-8"))
        except (ConnectionAbortedError, BrokenPipeError):
            pass

    def log_message(self, formato, *args):
        print(formato % args)


# =============================================
# INICIAR EL SERVIDOR
# =============================================

if __name__ == "__main__":
    print("====================================")
    print("  SMART CONTROLLER - Servidor Serial")
    print("====================================")
    print("")
    print("Escuchando en http://localhost:" + str(PUERTO_HTTP))
    print("Puertos serie configurados:")
    for puerto in PUERTOS_SERIE:
        print("  - " + puerto)
    print("")
    print("Conectando puertos serie...")
    try:
        abrir_todos_los_puertos()
    except ImportError:
        print("  ERROR: pyserial no instalado. Ejecuta: pip install pyserial")
    except Exception as e:
        print("  ERROR: " + str(e))

    print("")
    if puertos_abiertos:
        print("Conectado(s):")
        for p in puertos_abiertos:
            try:
                print("  - " + p.port)
            except:
                pass
    else:
        print("ATENCION: No se pudo abrir ningun puerto serie")
        print("Conecta el Arduino por USB o Bluetooth y reinicia el servidor")
    print("")
    print("Endpoints:")
    print("  GET  /          - Estado del servidor y puertos")
    print("  POST /enviar    - Enviar comando al Arduino")
    print("  POST /escanear  - Escanear codigo desde el Arduino")
    print("")
    print("Presiona Ctrl+C para salir")
    print("")

    servidor = ThreadingHTTPServer(("0.0.0.0", PUERTO_HTTP), Manejador)

    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("")
        print("Servidor detenido")
        cerrar_todos_los_puertos()
        servidor.server_close()
