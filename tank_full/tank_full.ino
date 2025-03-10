#include <IRremote.h>

#define IR_SEND_PIN PA12

IRsend IrSender;  // Correct object name

const int potPins[] = {A0, A1, A4, A5, A6, A7};  // Potentiometer pins
float potValues[6] = {0};  // Store mapped potentiometer values

// Define min and max ranges for mapping
const float RANGES_MIN[] = {0, -40, 0, 0, 0, -40};  
const float RANGES_MAX[] = {70, 80, 3, 3, 70, 80};  

void readPotentiometers() {
    for (int i = 0; i < 6; i++) {
        int rawValue = analogRead(potPins[i]);  
        float mappedValue = map(rawValue, 0, 1023, RANGES_MIN[i] * 100, RANGES_MAX[i] * 100) / 100.0;  
        potValues[i] = constrain(mappedValue, RANGES_MIN[i], RANGES_MAX[i]);  
    }
}

void setup() {
    Serial.begin(9600);
    IrSender.begin(IR_SEND_PIN);
    Serial.println("IR Sender Initialized");
}

void sendMessage(float tank_capacity, float max_press, float max_temp, 
                 float tank_pressure, float tank_temp, float current_gas, String vehicle_id) {
    
    String message = "|" + String(tank_capacity) + "," +
                     String(max_press) + "," +
                     String(max_temp) + "," +
                     String(tank_pressure) + "," +
                     String(tank_temp) + "," +
                     String(current_gas) + "," +
                     vehicle_id;
    
    Serial.println("Sending: " + message);
    
    for (size_t i = 0; i < message.length(); i++) {
        uint16_t asciiValue = (uint16_t)message[i];
        Serial.print(asciiValue);
        Serial.print(" ");
        IrSender.sendNEC(asciiValue, 32);
        delay(300);
    }
    Serial.println("\nMessage sent!");
}

void loop() {
    readPotentiometers(); 

    // Use mapped potentiometer values instead of pin numbers
    float tank_capacity = potValues[0];
    float max_press = potValues[1];
    float max_temp = potValues[2];
    float tank_pressure = potValues[3];
    float tank_temp = potValues[4];
    float current_gas = potValues[5];
    String vehicle_id = "JPN999";

    sendMessage(tank_capacity, max_press, max_temp, tank_pressure, tank_temp, current_gas, vehicle_id);
    
    delay(2000);  // Delay before sending again
}
