#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

// WiFi credentials
const char* ssid = "pad6";
const char* password = "nazzhannn";

// Firestore URL
String dataURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/data_received/rgA6HYWnkgtFb1MCZwLy";

// Serial Communication with Nucleo
HardwareSerial MySerial(2);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    MySerial.begin(115200, SERIAL_8N1, 16, 17);
    Serial.println("Serial2 initialized.");
}

void loop() {
    if (MySerial.available()) {
        String rawData = MySerial.readStringUntil('\n');
        Serial.println("Received: " + rawData);
        
        String values[15];
        parseData(rawData, values);
        
        sendToFirestore(values);
    }
    delay(300);
}

void parseData(String rawData, String values[]) {
    int index = 0;
    int start = 0;
    for (int i = 0; i < rawData.length(); i++) {
        if (rawData[i] == ',' || i == rawData.length() - 1) {
            if (i == rawData.length() - 1) i++;
            values[index++] = rawData.substring(start, i);
            start = i + 1;
            if (index >= 15) break;
        }
    }
}

void sendToFirestore(String values[]) {
    String jsonData = "{\"fields\": {"
                      "\"TANK_CAPACITY\": {\"doubleValue\": " + values[0] + "},"
                      "\"MAX_PRESS\": {\"doubleValue\": " + values[1] + "},"
                      "\"MAX_TEMP\": {\"doubleValue\": " + values[2] + "},"
                      "\"TANK_PRESSURE\": {\"doubleValue\": " + values[3] + "},"
                      "\"TANK_TEMP\": {\"doubleValue\": " + values[4] + "},"
                      "\"CURRENT_GAS\": {\"doubleValue\": " + values[5] + "},"
                      "\"SOC\": {\"doubleValue\": " + values[6] + "},"
                      "\"STATUS\": {\"stringValue\": \"" + values[7] + "\"},"
                      "\"VEHICLE_ID\": {\"stringValue\": \"" + values[8] + "\"},"
                      "\"MAX_DELIVERY_PRESS\": {\"doubleValue\": " + values[9] + "},"
                      "\"PRECOOL_TEMP\": {\"doubleValue\": " + values[10] + "},"
                      "\"FLOW_RATE_LIMIT\": {\"doubleValue\": " + values[11] + "},"
                      "\"FLOW_RAT\": {\"doubleValue\": " + values[12] + "},"
                      "\"DELIVERY_PRESS\": {\"doubleValue\": " + values[13] + "},"
                      "\"DELIVERY_TEMP\": {\"doubleValue\": " + values[14] + "}"
                      "}}";
    sendFirestoreRequest(dataURL, jsonData);
}

void sendFirestoreRequest(String url, String jsonData) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.PATCH(jsonData);
    if (httpCode > 0) {
        Serial.println("Response: " + http.getString());
    } else {
        Serial.println("Error: " + String(httpCode));
    }
    http.end();
}
