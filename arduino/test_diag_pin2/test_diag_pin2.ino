// Diagnostico: monitoreo directo del pin 2
#define RX_PIN 2

void setup() {
  Serial.begin(115200);
  pinMode(RX_PIN, INPUT);
  Serial.println("MONITOREO_DIRECTO_PIN_2");
  Serial.println("Presiona el mando RF...");
}

unsigned long last = 0;
bool lastState = LOW;
unsigned long changes = 0;

void loop() {
  bool state = digitalRead(RX_PIN);
  unsigned long now = micros();
  
  if (state != lastState) {
    changes++;
    if (changes % 2 == 0) {
      Serial.println(now - last);
    }
    last = now;
    lastState = state;
    
    if (changes > 1000) {
      Serial.println("--- DEMASIADOS CAMBIOS (ruido?) ---");
      changes = 0;
    }
  }
}
