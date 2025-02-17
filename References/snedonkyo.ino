#include <IRremote.h> 

#define IR_SEND_PIN PA12

IRsend IrSend(IR_SEND_PIN);

void setup() {
  Serial.begin(9600);
  IrSender.begin(IR_SEND_PIN);
  Serial.println("IR Sender Initialized");
}

void loop() {
  const char *message = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // Message to send
  Serial.print("Sending ASCII values: ");
  
  for (size_t i = 0; i < strlen(message); i++) {
    uint16_t asciiValue = (uint16_t)message[i]; // Get ASCII value
    Serial.print(asciiValue);
    Serial.print(" ");
    
    // Send the ASCII value using NEC protocol
    IrSend.sendNEC(asciiValue, 32); // Send with NEC protocol and 32-bit length
    delay(300); // Delay between each character
  }
  
  Serial.println("\nMessage sent!");
  delay(2000); // Delay before sending again
}
