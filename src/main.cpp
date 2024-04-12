//eigene MAC Adresse: 08:f9:e0:73:35:96
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <Wire.h> // Einbinden der Wire-Bibliothek für I2C-Kommunikation

const int pcf8575_address = 0x20; // I2C-Adresse des PCF8575
uint8_t receiverMac[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

typedef struct {
  unsigned long onTimes[16];  // Array von Zeiten für jedes Relais
} RelayControl;

RelayControl relayControl; // Instanz von RelayControl für die Speicherung der empfangenen Daten

// Struktur, um den Zustand und die Zeitsteuerung für jedes Relais zu speichern
typedef struct {
  bool isOn;                 // Zustand des Relais: ein oder aus
  unsigned long startTime;   // Startzeit, wenn das Relais eingeschaltet wird
  unsigned long onDuration;  // Dauer, wie lange das Relais eingeschaltet bleiben soll
} RelayState;

RelayState relayStates[16]; // Array von RelayState für jedes Relais
uint16_t relayBitmask = 0;  // Bitmaske für den Zustand aller Relais

void pcf8575Write(uint16_t data) {
  Wire.beginTransmission(pcf8575_address);
  Wire.write(data & 0xFF); // Niederwertiges Byte senden
  Wire.write(data >> 8);   // Höherwertiges Byte senden
  Wire.endTransmission();
}

void setRelayState(int relayNumber, bool state) {
  if (state) {
    relayBitmask |= (1 << relayNumber); // Relais einschalten
  } else {
    relayBitmask &= ~(1 << relayNumber); // Relais ausschalten
  }
  pcf8575Write(relayBitmask);
}

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  memcpy(&relayControl, data, sizeof(relayControl));
  for (int i = 0; i < 16; i++) {
    if (relayControl.onTimes[i] > 0) {
      relayStates[i].isOn = true;
      relayStates[i].startTime = millis();
      relayStates[i].onDuration = relayControl.onTimes[i];
      setRelayState(i, true); // Relais sofort einschalten
    }
  }
}

void setup() {
  Wire.begin(); // Starten der I2C-Kommunikation
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    return;
  }
  // Register callback function for received data
  esp_now_register_recv_cb(OnDataRecv);
  
  // Alle Relais ausschalten zu Beginn
  relayBitmask = 0;
  pcf8575Write(relayBitmask);

}

void loop() {
  unsigned long currentMillis = millis();
  for (int i = 0; i < 16; i++) {
    if (relayStates[i].isOn && currentMillis - relayStates[i].startTime >= relayStates[i].onDuration) {
      setRelayState(i, false); // Relais ausschalten
      relayStates[i].isOn = false; // Zustand aktualisieren
    }
  }
}



