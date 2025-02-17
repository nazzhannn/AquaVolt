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

    // Convert rawValue to actual range
    float mappedValue = ((rawValue / 1023.0) * (RANGES_MAX[i] - RANGES_MIN[i])) + RANGES_MIN[i];

    // Apply smoothing filter
    filteredPotValues[i] = (ALPHA * mappedValue) + ((1 - ALPHA) * filteredPotValues[i]);

    // Shift history array and store new value
    for (int j = AVERAGE_WINDOW - 1; j > 0; j--) {
      potHistory[i][j] = potHistory[i][j - 1];
    }
    potHistory[i][0] = filteredPotValues[i];

    // Calculate moving average
    potValues[i] = averagePotValue(i);
  }

  // Process SOC separately and use floating-point mapping
  float rawSOC = analogRead(potPins[5]);
  float processedSOC = (rawSOC / 1023.0) * 100.0;  // Convert raw ADC value to percentage
  String status = determineStatus(processedSOC);


  updateLCD(processedSOC, status);

  // Send all data to Serial Monitor
  Serial.print("{");
  Serial.print("\"TankPressure\":"); Serial.print(potValues[0]); Serial.print(",");
  Serial.print("\"DeliveryPressure\":"); Serial.print(potValues[1]); Serial.print(",");
  Serial.print("\"TankTemp\":"); Serial.print(potValues[2]); Serial.print(",");
  Serial.print("\"DeliveryTemp\":"); Serial.print(potValues[3]); Serial.print(",");
  Serial.print("\"FlowRate\":"); Serial.print(potValues[4]); Serial.print(",");
  Serial.print("\"SOC\":"); Serial.print(processedSOC, 1);
  Serial.println("}");

  // Ensure correct data is sent to ESP32
  MySerial.print(potValues[0], 2); MySerial.print(",");
  MySerial.print(potValues[1], 2); MySerial.print(",");
  MySerial.print(potValues[2], 2); MySerial.print(",");
  MySerial.print(potValues[3], 2); MySerial.print(",");
  MySerial.print(potValues[4], 2); MySerial.print(",");
  MySerial.println(processedSOC, 2); // Send with 2 decimal precision

  delay(1000);
}

void updateLCD(float soc, String status) {
  lcd.clear();

  // Row 1: Pressure readings
  lcd.setCursor(0, 0);
  lcd.print("P:"); lcd.print(potValues[0], 2); lcd.print(" MPa");
  lcd.print("DP:"); lcd.print(potValues[1], 2); lcd.print(" MPa");

  // Row 2: Temperature readings
  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print(potValues[2], 1); lcd.print("C");
  lcd.print("DT:"); lcd.print(potValues[3], 1); lcd.print("C");

  // Row 3: Flow Rate
  lcd.setCursor(0, 2);
  lcd.print("FlowRate:"); lcd.print(potValues[4], 2); lcd.print("kg/min");

  // Row 4: SOC and Status
  lcd.setCursor(0, 3);
  lcd.print("SOC:"); lcd.print(soc, 1); lcd.print("%");
  lcd.print("Status:"); lcd.print(status);
}
