#include <Arduino.h>
#include <IRremote.hpp>

#define IR_RECEIVE_PIN PA11

// Define the message and command mapping
const String message = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

char mapCommandToCharacter(uint16_t command) {
  // The command-to-character mapping
  switch (command) {
    case 0x8200: return 'A';
    case 0x4200: return 'B';
    case 0xC200: return 'C';
    case 0x2200: return 'D';
    case 0xA200: return 'E';
    case 0x6200: return 'F';
    case 0xE200: return 'G';
    case 0x1200: return 'H';
    case 0x9200: return 'I';
    case 0x5200: return 'J';
    case 0xD200: return 'K';
    case 0x3200: return 'L';
    case 0xB200: return 'M';
    case 0x7200: return 'N';
    case 0xF200: return 'O';
    case 0xA00: return 'P';
    case 0x8A00: return 'Q';
    case 0x4A00: return 'R';
    case 0xCA00: return 'S';
    case 0x2A00: return 'T';
    case 0xAA00: return 'U';
    case 0x6A00: return 'V';
    case 0xEA00: return 'W';
    case 0x1A00: return 'X';
    case 0x9A00: return 'Y';
    case 0x5A00: return 'Z';
    case 0xC00: return '0';
    case 0x8C00: return '1';
    case 0x4C00: return '2';
    case 0xCC00: return '3';
    case 0x2C00: return '4';
    case 0xAC00: return '5';
    case 0x6C00: return '6';
    case 0xEC00: return '7';
    case 0x1C00: return '8';
    case 0x9C00: return '9';
    default: return '?'; // Unknown command
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ; // Wait for Serial to become available.

  Serial.println(F("START"));

  // Start the receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.println(F("Ready to receive IR signals"));
}

void loop() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      // Map the command to a character and print
      char mappedCharacter = mapCommandToCharacter(IrReceiver.decodedIRData.command);
      Serial.println(mappedCharacter);
    }
    IrReceiver.resume(); // Ready for the next IR signal
  }
  delay(100); // Wait a bit before next loop
}
