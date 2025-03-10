
#include <Arduino.h>
#include <IRremote.hpp>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define IR_RECEIVE_PIN PA11

String receivedData = ""; // Temporary storage for received characters
float tank_capacity = 0.0, max_press = 0.0, max_temp = 0.0, tank_pressure = 0.0, tank_temp = 0.0, current_gas = 0.0;
String vehicle_id = "";
int dataIndex = -1;

const int potPins[] = {A0, A1, A4, A5, A6, A7};
float potValues[6] = {0};

HardwareSerial MySerial(PA10, PA9);
const float RANGES_MIN[] = {0, -40, 0, 0, 0, -40}; //mxp,pcl,frl,fr,dp,dt
const float RANGES_MAX[] = {70, 80, 3, 3, 70, 80};

LiquidCrystal_I2C lcd(0x27, 20, 4);

const String station_id = "Stesen Jalan Pudu";
const String dispenser_type = "H2O";
const String location = "Pudu";

char mapCommandToCharacter(uint16_t command) {
  switch (command) {
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
    case 0x3400: return ',';
    case 0x7400: return '.';
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
    case 0x3E00: return '|';
    default: return '?';
  }
}

void updateLCD(float soc, String status);
bool startReceiving = false;
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("START"));
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("Ready to receive IR signals"));

  MySerial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

unsigned long lastDecodeTime = 0;  // Store the time of last IR decode
const unsigned long ACTIVE_DURATION = 2000;  // 2 seconds for active status
const unsigned long NOZZLE_TIMEOUT = 9000;  // 5 seconds timeout for nozzle detached
bool nozzle_detached = false;  // Tracks if the nozzle is detached
bool loadingActive = false;      // Flag to track if loading is active
unsigned long loadingStartTime;  // Stores the start time of loading
bool firstIRReceived = false;

void loadingLCD() {
    lcd.clear();
    lcd.setCursor(5, 1);
    lcd.print("Loading");

    loadingStartTime = millis();
    loadingActive = true;
    firstIRReceived = true;
}

void updateLoadingAnimation() {
    if (loadingActive) {
        lcd.setCursor(13, 1);
        int dots = (millis() / 500) % 4; // Change every 500ms
        for (int i = 0; i < dots; i++) {
            lcd.print(".");
        }
        lcd.print("   ");  // Clear previous dots
    }
}


void loop() {
    String active_status = "Inactive";
    String activeat = "";

    // Check if IR signal is received
    if (IrReceiver.decode()) {
        lastDecodeTime = millis();  // Update last received time
        active_status = "Active";
        activeat = station_id;

        if (!firstIRReceived) { 
            loadingLCD(); // Start the 30s loading display
            firstIRReceived = true;  // Mark that first IR is received
        }

        // Start loading only if it's not already active
      

        // Process IR data
        if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
            char receivedChar = mapCommandToCharacter(IrReceiver.decodedIRData.command);

            if (receivedChar == '|') {  
                receivedData = "";
                dataIndex = 0;
                startReceiving = true;
            } 
            else if (startReceiving) { 
                if (receivedChar == ',') {
                    if (dataIndex == 0) tank_capacity = receivedData.toFloat();
                    else if (dataIndex == 1) max_press = receivedData.toFloat();
                    else if (dataIndex == 2) max_temp = receivedData.toFloat();
                    else if (dataIndex == 3) tank_pressure = receivedData.toFloat();
                    else if (dataIndex == 4) tank_temp = receivedData.toFloat();
                    else if (dataIndex == 5) current_gas = receivedData.toFloat();
                    receivedData = "";
                    dataIndex++;
                } 
                else { 
                    receivedData += receivedChar;
                }
            }
        }
        IrReceiver.resume();
    }

    // Stop the loading after 30 seconds
    if (loadingActive && millis() - loadingStartTime >= 30000) {
        loadingActive = false;
        lcd.clear();  // Clear LCD
    }

    if (loadingActive) {
    updateLoadingAnimation();
    }


    // Nozzle detachment logic
    if (millis() - lastDecodeTime > NOZZLE_TIMEOUT) {
        nozzle_detached = true;
    } else {
        nozzle_detached = false;
    }

    // Maintain "Active" status for 2 seconds after last IR signal
    if (millis() - lastDecodeTime < ACTIVE_DURATION) {
        active_status = "Active";
        activeat = station_id;
    } else {
        active_status = "Inactive";
        activeat = "";
    }

    // Reset system if nozzle is detached
    if (nozzle_detached) {
        NVIC_SystemReset();
    }

    // Calculate SOC
    float soc = (current_gas / tank_capacity) * 100;
    if (isnan(soc) || tank_capacity == 0) {
        soc = 0;
    } else if (soc > 100) {
        soc = 100;
    } else if (soc < 0) {
        soc = 0;
    }

    // Determine status
    String status = "Available";
    if (soc > 95) {
        status = "Completed";
    } 
    else if (soc == 0) {
        status = "Available";
    } 
    else {
        status = "Fueling";
    }

    // Store vehicle_id if receiving data
    if (startReceiving && dataIndex == 6) {
        vehicle_id = receivedData;
    }

    // Read potentiometer values
    for (int i = 0; i < 6; i++) {
        potValues[i] = map(analogRead(potPins[i]), 0, 1023, RANGES_MIN[i] * 100, RANGES_MAX[i] * 100) / 100.0;
        potValues[i] = constrain(potValues[i], RANGES_MIN[i], RANGES_MAX[i]);
    }

    // Send data to Serial
    Serial.print("{");
    Serial.print("\"TankCapacity\":"); Serial.print(tank_capacity, 2); Serial.print(",");
    Serial.print("\"MaxPress\":"); Serial.print(max_press, 2); Serial.print(",");
    Serial.print("\"MaxTemp\":"); Serial.print(max_temp, 2); Serial.print(",");
    Serial.print("\"TankPressure\":"); Serial.print(tank_pressure, 2); Serial.print(",");
    Serial.print("\"TankTemp\":"); Serial.print(tank_temp, 2); Serial.print(",");
    Serial.print("\"CurrentGas\":"); Serial.print(current_gas, 2); Serial.print(",");
    Serial.print("\"SOC\":"); Serial.print(soc, 2); Serial.print(",");
    Serial.print("\"Status\":\""); Serial.print(status); Serial.print("\",");
    Serial.print("\"VehicleID\":\""); Serial.print(vehicle_id); Serial.print("\",");
    Serial.print("\"MaxDeliveryPress\":"); Serial.print(potValues[0], 2); Serial.print(",");
    Serial.print("\"PrecoolTemp\":"); Serial.print(potValues[1], 2); Serial.print(",");
    Serial.print("\"FlowRateLimit\":"); Serial.print(potValues[2], 2); Serial.print(",");
    Serial.print("\"FlowRate\":"); Serial.print(potValues[3], 2); Serial.print(",");
    Serial.print("\"DeliveryPress\":"); Serial.print(potValues[4], 2); Serial.print(",");
    Serial.print("\"DeliveryTemp\":"); Serial.print(potValues[5], 2); Serial.print(",");
    Serial.print("\"StationID\":\""); Serial.print(station_id); Serial.print("\",");
    Serial.print("\"DispenserType\":\""); Serial.print(dispenser_type); Serial.print("\",");
    Serial.print("\"Active_status\":\""); Serial.print(active_status); Serial.print("\",");
    Serial.print("\"Activeat\":\""); Serial.print(activeat); Serial.print("\",");
    Serial.print("\"Location\":\""); Serial.print(location); Serial.println("\"}");

    // Send to MySerial
    MySerial.print("&"); 
    MySerial.print(tank_capacity, 2); MySerial.print(",");
    MySerial.print(max_press, 2); MySerial.print(",");
    MySerial.print(max_temp, 2); MySerial.print(",");
    MySerial.print(tank_pressure, 2); MySerial.print(",");
    MySerial.print(tank_temp, 2); MySerial.print(",");
    MySerial.print(current_gas, 2); MySerial.print(",");
    MySerial.print(soc, 2); MySerial.print(",");
    MySerial.print(status); MySerial.print(",");
    MySerial.print(vehicle_id); MySerial.print(",");
    MySerial.print(potValues[0], 2); MySerial.print(",");
    MySerial.print(potValues[1], 2); MySerial.print(",");
    MySerial.print(potValues[2], 2); MySerial.print(",");
    MySerial.print(potValues[3], 2); MySerial.print(",");
    MySerial.print(potValues[4], 2); MySerial.print(",");
    MySerial.print(potValues[5], 2); MySerial.print(",");
    MySerial.print(station_id); MySerial.print(",");
    MySerial.print(dispenser_type); MySerial.print(",");
    MySerial.print(active_status); MySerial.print(",");
    MySerial.print(activeat); MySerial.print(",");
    MySerial.print(location);
    MySerial.println("$");

    updateLCD(soc, status);
    delay(100);
}





void updateLCD(float soc, String status) {
if (loadingActive) return;

  lcd.clear();

  // Line 1: Max Delivery Press and Delivery Pressure
  lcd.setCursor(0, 0);
  lcd.print("MxP:");
  lcd.print(potValues[0], 1);
  lcd.print(" DlvP:");
  lcd.print(potValues[4], 1);

  // Line 2: Precool Temp and Delivery Temp
  lcd.setCursor(0, 1);
  lcd.print("PclT:");
  lcd.print(potValues[1], 1);
  lcd.print(" DlvT:");
  lcd.print(potValues[5], 1);

  // Line 3: Flow Rate Limit and Flow Rate
  lcd.setCursor(0, 2);
  lcd.print("FRL:");
  lcd.print(potValues[2], 1);
  lcd.print(" FR:");
  lcd.print(potValues[3], 1);

  // Line 4: SOC and Status
  lcd.setCursor(0, 3);
  lcd.print("SOC:");
  lcd.print(soc, 1);
  lcd.print("% ");
  lcd.print(status);
}