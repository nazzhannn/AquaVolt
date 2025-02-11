//NOZZLE’S

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define potentiometer pins
const int potPins[] = {A0, A1, A4, A5, A6, A7};
float potValues[6] = {0};

// Define custom serial pins for Nucleo (PA10: RX, PA9: TX)
HardwareSerial MySerial(PA10, PA9);  // Create a HardwareSerial object for communication

// Define ranges for each parameter
const float RANGES_MIN[] = {0, 0, -40, -40, 0, 0};   // Min values
const float RANGES_MAX[] = {70, 70, 85, 85, 3, 100}; // Max values

// Initialize the LCD (address 0x27, 16x2 display)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address is usually 0x27 or 0x3F

void setup() {
  // Initialize Serial for USB Serial Monitor
  Serial.begin(9600);
  while (!Serial);  // Wait for serial connection

  // Print a startup message to check if Serial is working
  Serial.println("Serial Monitor Initialized");

  // Initialize Serial for ESP32 communication
  MySerial.begin(115200);

  // Initialize LCD
  lcd.init();       // Initialize LCD
  lcd.backlight();  // Turn on LCD backlight
  lcd.clear();      // Clear the LCD screen

  // Initialize potentiometer pins
  for (int i = 0; i < 6; i++) {
    pinMode(potPins[i], INPUT);
  }

  // Confirmation message
  Serial.println("Nozzle Nucleo Initialized");
}

float filteredPotValues[6] = {0};
const float ALPHA = 0.1; // Smoothing factor
const int AVERAGE_WINDOW = 5;
int potHistory[6][AVERAGE_WINDOW]; // History for averaging

float averagePotValue(int index) {
  float sum = 0;
  for (int i = 0; i < AVERAGE_WINDOW; i++) {
    sum += potHistory[index][i];
  }
  return sum / AVERAGE_WINDOW;
}

float processSOC(float rawSOC) {
  // Scale the SOC based on the potentiometer range
  float minSOC = 0;      // Minimum SOC value
  float maxSOC = 100;    // Maximum SOC value (or adjust to your needs)
  
  // Convert raw value to SOC percentage
  float soc = map(rawSOC, 0, 1023, minSOC * 100, maxSOC * 100) / 100.0;  // Map and scale to desired range
  return soc;
}

// Function to determine the status based on SOC
String determineStatus(float soc) {
  if (soc >= 90) {
    return "Completed";
  } else if (soc > 0) {
    return "Fueling";
  } else {
    return "Error";
  }
}

void loop() {
  // Read potentiometers and apply smoothing and averaging
  for (int i = 0; i < 6; i++) {
    int rawValue = analogRead(potPins[i]); // Read ADC value

    // Apply smoothing to the potentiometer value
    filteredPotValues[i] = ALPHA * (rawValue / 1023.0 * (RANGES_MAX[i] - RANGES_MIN[i]) + RANGES_MIN[i]) +
                          (1 - ALPHA) * filteredPotValues[i];
    potValues[i] = filteredPotValues[i];

    // Shift the history array to make room for the new value
    for (int j = AVERAGE_WINDOW - 1; j > 0; j--) {
      potHistory[i][j] = potHistory[i][j - 1];
    }
    potHistory[i][0] = rawValue; // Add the new value to the history

    // Calculate the averaged potentiometer value
    potValues[i] = averagePotValue(i);
  }

  // Process the SOC value to calculate status
  float processedSOC = processSOC(potValues[5]);
  String status = determineStatus(processedSOC);

  // Update LCD with SOC and status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SOC: ");
  lcd.print(processedSOC);
  lcd.print("%");

  lcd.setCursor(0, 1); // Move to the second line
  lcd.print("Status: ");
  lcd.print(status);

  // Send all data to the Serial Monitor
  Serial.print("{");
  Serial.print("\"TankPressure\":"); Serial.print(potValues[0]); Serial.print(",");
  Serial.print("\"DeliveryPressure\":"); Serial.print(potValues[1]); Serial.print(",");
  Serial.print("\"TankTemp\":"); Serial.print(potValues[2]); Serial.print(",");
  Serial.print("\"DeliveryTemp\":"); Serial.print(potValues[3]); Serial.print(",");
  Serial.print("\"FlowRate\":"); Serial.print(potValues[4]); Serial.print(",");
  Serial.print("\"SOC\":"); Serial.print(processedSOC);  
  Serial.println("}");

  // Prepare a comma-separated string of numbers
  String data = String(potValues[0]) + "," +
                String(potValues[1]) + "," +
                String(potValues[2]) + "," +
                String(potValues[3]) + "," +
                String(potValues[4]) + "," +
                String(potValues[5]);

  // Send the data to the ESP32
  MySerial.println(data); // Send it over serial

  delay(1000); // Wait 1 second between messages
}
