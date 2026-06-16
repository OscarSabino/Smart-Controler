# GUIA DE MONTAJE - Smart Controller

## Componentes necesarios

| Cantidad | Componente |
|----------|------------|
| 1 | Arduino UNO |
| 1 | Modulo Bluetooth HC-05 |
| 1 | Receptor IR (TSOP38238 o similar) |
| 1 | LED IR emisor |
| 1 | Modulo RF 433MHz (receptor + emisor) |
| 1 | Resistencia 2kΩ |
| 1 | Resistencia 1kΩ |
| 1 | LED comun (opcional, para feedback) |
| 1 | Resistencia 220Ω (para el LED) |
| Varios | Cables Dupont macho-macho y macho-hembra |
| 1 | Breadboard |

---

## ASIGNACION DE PINES (Arduino)

```
Pin 6  → HC-05 TX (entrada serial)
Pin 7  → HC-05 RX (salida serial, con divisor de tensión)
Pin 8  → Receptor IR (señal)
Pin 9  → LED IR emisor (ánodo)
Pin 11 → RF Receptor (DATA)
Pin 12 → RF Emisor (DATA)
```

---

## 1. RECEPTOR IR (Pin 8)

El receptor IR tiene 3 patitas. Mirando la cara plana (con la marca hacia ti):

```
  _______
 |  ○ ○  |   Cara plana hacia ti
 |_ ○ ___|
   │ │ │
   │ │ └─── Pin 3: Señal (OUT) → Pin 8 Arduino
   │ └───── Pin 2: GND → GND Arduino
   └─────── Pin 1: VCC → 5V Arduino
```

**Conexiones:**
- Patita izquierda (VCC) → 5V Arduino
- Patita centro (GND) → GND Arduino
- Patita derecha (OUT) → Pin 8 Arduino

---

## 2. LED IR EMISOR (Pin 9)

El LED IR se parece a un LED normal pero es transparente/azulado.

**Conexiones:**
- Ánodo (pata larga) → Pin 9 Arduino (con resistencia 220Ω en serie)
- Cátodo (pata corta) → GND Arduino

Opcional: también puedes conectarlo directo a Pin 9 (el pin PWM ya limita corriente), pero es mejor con resistencia.

---

## 3. MODULO RF 433MHz

### Emisor (TX)
```
Pin 12 Arduino → DATA del emisor
5V Arduino    → VCC del emisor
GND Arduino   → GND del emisor
```

### Receptor (RX)
```
Pin 11 Arduino → DATA del receptor
5V Arduino    → VCC del receptor
GND Arduino   → GND del receptor
```

---

## 4. MODULO BLUETOOTH HC-05

El HC-05 funciona a 3.3V en su pin RX. Hay que hacer un **divisor de tensión** para bajar los 5V del pin 7 del Arduino a ~3.3V.

### Divisor de tensión para RX del HC-05:

```
     Arduino Pin 7 ──── 2kΩ ────┬──── RX del HC-05
                                 │
                                1kΩ
                                 │
                                GND
```

### Conexiones completas:

| HC-05 | Conexión |
|-------|----------|
| VCC   | 5V Arduino |
| GND   | GND Arduino |
| TX    | Directo a Pin 6 Arduino |
| RX    | A través del divisor (ver arriba) desde Pin 7 Arduino |
| KEY   | Dejarlo desconectado (solo se usa para modo AT) |

> **El HC-05 necesita 5V en VCC, NO 3.3V.** El divisor solo va en el pin RX.

---

## 5. VISTA COMPLETA (Breadboard)

```
                    ARDUINO UNO
    ┌───────────────────────────────────────┐
    │                                       │
    │  (RX) 0    ○ ○   5V ────── HC-05 VCC  │
    │  (TX) 1    ○ ○   GND ─ GND compartido │
    │        2    ○ ○                       │
    │        3    ○ ○                       │
    │        4    ○ ○                       │
    │        5    ○ ○                       │
    │        6 ←─── HC-05 TX (directo)      │
    │        7 ──→ 2kΩ ─┬─→ HC-05 RX        │
    │                   │                   │
    │        8 ←─── IR Receiver (OUT)       │
    │        9 ──→ 220Ω ──→ LED IR (ánodo)  │
    │       10    ○ ○  1kΩ                  │
    │                   │                   │
    │       11 ←─── RF Receptor (DATA)      │
    │       12 ──→ RF Emisor (DATA)         │
    │       13    ○ ○  GND                  │
    └───────────────────────────────────────┘

    IMPORTANTE: GND comun entre TODOS los modulos
```

---

## 6. VERIFICACION RAPIDA

Después de montar todo y subir el firmware:

1. Abre el **Monitor Serie** a 9600 baud
2. Deberías ver: `OK:INICIADO`
3. Escribe `TEST` y presiona Enter → responde `OK:TEST`
4. Apunta un mando IR al receptor → aparece `IR:FFAABBCC` (o similar)
5. Escribe `SEND_IR` → reenvía el último código capturado

Para probar con el servidor Python:
```
python servidor_bt.py
```
Luego desde la web: `http://localhost/Smart-Controler/web/`

---

## 7. SOLUCION DE PROBLEMAS

| Fallo            | Causa probable |
|------------------|----------------|
| No aparece `OK:INICIADO` | Velocidad de Monitor Serie incorrecta (debe ser 9600) |
| No capta IR      | Receptor IR en pin 8 incorrecto, polaridad invertida, o GND faltante |
| No envía IR      | LED IR emisor en pin 9 quemado o resistencia muy alta |
| HC-05 no conecta | Divisor de tensión mal hecho, o RX/TX invertidos |
| RF no funciona   | Antena faltante (un trozo de cable de 17cm ayuda) |
