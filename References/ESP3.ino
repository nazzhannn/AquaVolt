#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

// WiFi credentials
const char* ssid = "pad6";
const char* password = "nazzhannn";

// Firestore URLs for different collections
String vehicleURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/vehicle/XVk3vBoj8NdVzxELfJyK";
String stationURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/stations/cCLVpDRlPevrjgkYi6F0";
String paymentURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/payment/Bz7oD6c69viFnkF1shG8";

// Create HardwareSerial instance for communication
HardwareSerial MySerial(2);  // Serial2 with RX2: GPIO16, TX2: GPIO17

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  MySerial.begin(115200, SERIAL_8N1, 16, 17);  // RX: GPIO16, TX: GPIO17
  Serial.println("Serial2 initialized, ready to receive data from Nucleo.");
}

void loop() {
  if (MySerial.available()) {
    String rawData = MySerial.readStringUntil('\n');  // Read incoming data
    float values[6];
    parseData(rawData, values);

    // Send data to all collections
    sendToVehicle(values);
    sendToStation(values);
    sendToPayment(values);
  }

  delay(300);  // Avoid flooding requests
}

void parseData(String rawData, float values[]) {
  int index = 0;
  int start = 0;

  for (int i = 0; i < rawData.length(); i++) {
    if (rawData[i] == ',' || i == rawData.length() - 1) {
      if (i == rawData.length() - 1) i++; // Include last character
      values[index] = rawData.substring(start, i).toFloat();
      index++;
      start = i + 1;
    }
  }
}

void sendToVehicle(float values[]) {
  String jsonData = "{\"fields\": {"
                    "\"VEHICLE_ID\": {\"stringValue\": \"VHHHTEST\"},"
                    "\"FINAL_PRESSURE\": {\"doubleValue\": " + String(values[0]) + "},"
                    "\"FINAL_TEMP\": {\"doubleValue\": " + String(values[2]) + "},"
                    "\"FINAL_SOC\": {\"doubleValue\": " + String(values[5]) + "},"
                    "\"TOTAL_DISPENSED\": {\"doubleValue\": " + String(values[4]) + "}"
                    "}}";
  sendFirestoreRequest(vehicleURL, jsonData);
}

void sendToStation(float values[]) {
  String jsonData = "{\"fields\": {"
                    "\"STATION_ID\": {\"stringValue\": \"STN123\"},"
                    "\"DISPENSER_TYPE\": {\"stringValue\": \"H70\"},"
                    "\"MAX_DELIVERY_TEMPERATURE\": {\"doubleValue\": " + String(values[3]) + "},"
                    "\"PRECOOL_TEMP\": {\"stringValue\": \"-40C\"},"
                    "\"FLOWRATE_LIMIT\": {\"doubleValue\": " + String(values[4]) + "}"
                    "}}";
  sendFirestoreRequest(stationURL, jsonData);
}

void sendToPayment(float values[]) {
  String jsonData = "{\"fields\": {"
                    "\"ACTIVEAT\": {\"stringValue\": \"STESEN MELAWATI\"},"
                    "\"STATUS\": {\"stringValue\": \"COMPLETED\"},"
                    "\"VEHICLE_ID\": {\"stringValue\": \"VHHHTEST\"}"
                    "}}";
  sendFirestoreRequest(paymentURL, jsonData);
}

void sendFirestoreRequest(String url, String jsonData) {
  HTTPClient http;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PATCH(jsonData);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error: " + String(httpCode));
  }

  http.end();
}
