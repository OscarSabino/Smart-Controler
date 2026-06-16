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
ARCHIVO_SCAN = "scan_result.txt"
ARCHIVO_ENVIAR = "comando_enviado.txt"
ARCHIVO_RF_SCAN = "rf_scan_result.txt"
ARCHIVO_RF_ENVIAR = "rf_comando_enviado.txt"
ARCHIVO_IR_SCAN = "ir_scan_result.txt"
ARCHIVO_IR_ENVIAR = "ir_comando_enviado.txt"

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


def escribir_archivo(ruta, contenido):
    """Escribe contenido a un archivo (pisa si existe)"""
    try:
        with open(ruta, "w") as f:
            f.write(contenido)
    except Exception as e:
        print("Error al escribir " + ruta + ": " + str(e))


def limpiar_buffers():
    """Limpia buffers de entrada de todos los puertos"""
    for p in puertos_abiertos:
        try:
            if p and p.is_open:
                while p.in_waiting > 0:
                    p.readline()
        except:
            pass


ARCHIVOS = [
    ARCHIVO_SCAN,
    ARCHIVO_ENVIAR,
    ARCHIVO_RF_SCAN,
    ARCHIVO_RF_ENVIAR,
    ARCHIVO_IR_SCAN,
    ARCHIVO_IR_ENVIAR,
]


def crear_archivos_vacios():
    """Crea todos los archivos de texto vacios al iniciar"""
    for ruta in ARCHIVOS:
        escribir_archivo(ruta, "")
    print("Archivos de texto creados:")
    for ruta in ARCHIVOS:
        print("  - " + ruta)


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
            proto_num = datos.get("proto_num")
            bits = datos.get("bits")

            if not codigo:
                self.enviar_error(400, "Falta el codigo del comando")
                return

            # Si el codigo ya incluye el prefijo (ej. "IR:0x..."), no lo duplicamos
            if codigo.startswith("IR:") or codigo.startswith("RF:"):
                trama = codigo + "\n"
            elif protocolo.upper() == "IR":
                # Si tenemos proto_num y bits del frontend, usarlos
                if proto_num is not None and bits is not None:
                    trama = "IR:" + codigo + "," + str(proto_num) + "," + str(bits) + "\n"
                else:
                    # Buscar en el ultimo scan IR por si tiene proto/bits
                    trama = "IR:" + codigo + "\n"
                    try:
                        with open(ARCHIVO_IR_SCAN, "r") as f_scan:
                            scan_line = f_scan.read().strip()
                        if scan_line:
                            # scan_line es "IR:CODIGO,PROTO,BITS"
                            scan_parts = scan_line[3:].split(",")
                            if len(scan_parts) >= 3 and scan_parts[0] == codigo:
                                trama = scan_line + "\n"
                                print("Usando proto/bits del scan: " + trama.strip())
                    except:
                        pass
            else:
                trama = protocolo.upper() + ":" + codigo + "\n"

            # Escribir el comando al archivo general (pisando si existe)
            escribir_archivo(ARCHIVO_ENVIAR, trama)
            print("Comando escrito en " + ARCHIVO_ENVIAR + ": " + trama.strip())

            # Si es RF, escribir tambien al archivo RF especifico
            if trama.startswith("RF:") or protocolo.upper() == "RF":
                escribir_archivo(ARCHIVO_RF_ENVIAR, trama)
                print("Comando RF escrito en " + ARCHIVO_RF_ENVIAR + ": " + trama.strip())

            # Si es IR, escribir tambien al archivo IR especifico
            if trama.startswith("IR:") or protocolo.upper() == "IR":
                escribir_archivo(ARCHIVO_IR_ENVIAR, trama)
                print("Comando IR escrito en " + ARCHIVO_IR_ENVIAR + ": " + trama.strip())

            # Leer el archivo y enviar al Arduino
            try:
                with open(ARCHIVO_ENVIAR, "r") as f:
                    contenido = f.read().strip()
                if contenido:
                    enviar_a_arduino(contenido + "\n")
                    print("Enviado desde archivo: " + contenido)
                else:
                    enviar_a_arduino(trama)
                    print("Archivo vacio, enviando directo: " + trama.strip())
            except Exception as e:
                print("Error leyendo archivo, enviando directo: " + str(e))
                enviar_a_arduino(trama)

            self.enviar_json({
                "mensaje": "Senial enviada correctamente",
                "protocolo": protocolo,
                "codigo": codigo
            })

        except Exception as e:
            print("Error: " + str(e))
            self.enviar_error(500, "Error interno: " + str(e))

    # =============================================
    # ESCANEAR CODIGOS IR / RF
    # =============================================
    def escanear_codigo(self):
        p = puerto_activo()
        if not p:
            self.enviar_error(500, "No hay ningun puerto serie conectado")
            return

        # Pisa los archivos especificos (crea si no existen, pisa si existen)
        escribir_archivo(ARCHIVO_RF_SCAN, "")
        escribir_archivo(ARCHIVO_IR_SCAN, "")
        print("Archivos RF/IR creados/pisados. Esperando senales...")

        # Limpia buffer Serial
        time.sleep(0.2)
        try:
            while p.in_waiting > 0:
                p.readline()
        except:
            pass

        # Envia SCAN al Arduino
        try:
            p.write(b"SCAN\n")
        except Exception as e:
            self.enviar_error(500, "Error al escribir en puerto serie: " + str(e))
            return

        inicio = time.time()
        while time.time() - inicio < TIMEOUT_ESCANEO:
            # Lee lineas del Arduino
            try:
                while p.in_waiting > 0:
                    linea = p.readline().decode("utf-8").strip()
                    print("Arduino: " + linea)
                    if linea.startswith("RF:"):
                        escribir_archivo(ARCHIVO_RF_SCAN, linea + "\n")
                    elif linea.startswith("IR:"):
                        escribir_archivo(ARCHIVO_IR_SCAN, linea + "\n")
            except:
                pass

            # Comprueba si el archivo RF tiene contenido
            try:
                with open(ARCHIVO_RF_SCAN, "r") as f:
                    contenido = f.read().strip()
                if contenido:
                    partes = contenido[3:].split(",")
                    self.enviar_json({"codigo": partes[0], "protocolo": "RF"})
                    return
            except:
                pass

            # Comprueba si el archivo IR tiene contenido
            try:
                with open(ARCHIVO_IR_SCAN, "r") as f:
                    contenido = f.read().strip()
                if contenido:
                    partes = contenido[3:].split(",")
                    respuesta = {"codigo": partes[0], "protocolo": "IR"}
                    if len(partes) >= 3:
                        respuesta["proto_num"] = int(partes[1])
                        respuesta["bits"] = int(partes[2])
                    self.enviar_json(respuesta)
                    return
            except:
                pass

            time.sleep(0.05)

        # Tiempo agotado
        self.enviar_json({"codigo": "", "protocolo": ""})

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
    crear_archivos_vacios()
    print("Archivo de resultados: " + ARCHIVO_SCAN)
    print("Archivo de envio: " + ARCHIVO_ENVIAR)
    print("Archivo RF scan: " + ARCHIVO_RF_SCAN)
    print("Archivo RF envio: " + ARCHIVO_RF_ENVIAR)
    print("Archivo IR scan: " + ARCHIVO_IR_SCAN)
    print("Archivo IR envio: " + ARCHIVO_IR_ENVIAR)
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
