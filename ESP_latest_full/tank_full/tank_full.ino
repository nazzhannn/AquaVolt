#include <IRremote.h> 

#define IR_SEND_PIN PA12

IRsend IrSend(IR_SEND_PIN);

// Variables to store sensor data
float tank_capacity = 22;
float max_press = 70;
float max_temp = 80;
float tank_pressure = 52;
float tank_temp = 73;
float current_gas = 19;
String vehicle_id = "DEF456";

void setup() {
    Serial.begin(9600);
    IrSender.begin(IR_SEND_PIN);
    Serial.println("IR Sender Initialized");
}

void sendMessage() {
    String message = "|" + String(tank_capacity) + "," +
                     String(max_press) + "," +
                     String(max_temp) + "," +
                     String(tank_pressure) + "," +
                     String(tank_temp) + "," +
                     String(current_gas) + "," +
                     vehicle_id;
    Serial.println(message);
    
    for (size_t i = 0; i < message.length(); i++) {
        uint16_t asciiValue = (uint16_t)message[i];
        Serial.print(asciiValue);
        Serial.print(" ");
        IrSend.sendNEC(asciiValue, 32);
        delay(300);
    }
    Serial.println("\nMessage sent!");
}

void loop() {
    sendMessage(); // Send message in each loop
    delay(2000);   // Delay before sending again
}
