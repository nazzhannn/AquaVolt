#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

// WiFi credentials
const char* ssid = "pad6";
const char* password = "nazzhannn";

// Firestore URLs
String paymentURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/payment";
String stationURL = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents/stations/z2T0VxfdviBYnyyW78g2";

// Serial Communication with Nucleo
HardwareSerial MySerial(2);

bool parseData(String rawData, String values[]);
void updateStationDocument(String values[]);
void findAndUpdatePayment(String vehicle_id, String values[]);
void updatePaymentStatusToInactive(String lastVehicleID);
String getStationStatusFromFirestore();

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

        String values[20];
        if (!parseData(rawData, values)) {
            return;  // Stop execution if data is invalid
        }
        delay(300);
        updateStationDocument(values);
        
        static String lastVehicleID = "";  
        String vehicle_id = values[8];  
        static bool inactiveUpdated = false;

        if (vehicle_id != "") {
            lastVehicleID = vehicle_id;  
            inactiveUpdated = false;
            findAndUpdatePayment(vehicle_id, values);
        } else if (!inactiveUpdated && lastVehicleID != "") {
            Serial.println("Nozzle detached. Updating last vehicle's status to Inactive.");
            updatePaymentStatusToInactive(lastVehicleID);
            lastVehicleID = "";
            inactiveUpdated = true;
        }

        // 🔹 Fetch Station Status from Firestore
        String stationStatus = getStationStatusFromFirestore();
        if (stationStatus == "Fuelling") {
            MySerial.println("Approved");
            Serial.println("Sent 'Approved' to the serial.");
        }
    }
}

// 🔹 Function to Get Station Status from Firestore
String getStationStatusFromFirestore() {
    HTTPClient http;
    http.begin(stationURL);
    int httpCode = http.GET();
    String status = "";

    if (httpCode > 0) {
        String response = http.getString();
        int statusIndex = response.indexOf("\"STATUS\": {\"stringValue\": \"");
        if (statusIndex > 0) {
            statusIndex += 26;
            int endIndex = response.indexOf("\"", statusIndex);
            status = response.substring(statusIndex, endIndex);
        }
    }
    http.end();
    return status;
}

// 🔹 Function to Parse Data
bool parseData(String rawData, String values[]) {
    int startIdx = rawData.indexOf('&');
    int endIdx = rawData.indexOf('$');

    // Validate data format
    if (startIdx == -1 || endIdx == -1 || startIdx > endIdx) {
        Serial.println("Error: Invalid data format");
        return false;
    }

    // Extract data between '&' and '$'
    String extractedData = rawData.substring(startIdx + 1, endIdx);
    extractedData.trim();
    Serial.println("Extracted Data: " + extractedData);

    if (extractedData.length() == 0) {
        Serial.println("Error: Empty data");
        return false;
    }

    int index = 0, start = 0;
    int length = extractedData.length();

    for (int i = 0; i < length; i++) {
        if (extractedData[i] == ',' || i == length - 1) {
            if (i == length - 1) i++;

            String value = extractedData.substring(start, i);
            value.trim();

            if (index < 20) {
                values[index++] = value;
            } else {
                Serial.println("Warning: Too many fields, extra data ignored.");
                break;
            }
            start = i + 1;
        }
    }

    if (index < 20) {
        Serial.println("Warning: Missing values, filling remaining with empty strings.");
        for (int i = index; i < 20; i++) {
            values[i] = "";
        }
    }

    return true;
}

// 🔹 Function to Update Station Document
void updateStationDocument(String values[]) {
    String jsonData = "{\"fields\": {"
                      "\"TANK_CAPACITY\": {\"doubleValue\": " + String(values[0].toFloat(), 2) + "},"
                      "\"STATUS\": {\"stringValue\": \"" + values[7] + "\"}"
                      "}}";
    sendFirestoreRequest(stationURL, jsonData);
}

// 🔹 Function to Send HTTP Request to Firestore
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

// 🔹 Function to Find and Update Payment
void findAndUpdatePayment(String vehicle_id, String values[]) {
    String queryBody = "{"
                        "\"structuredQuery\": {"
                        "\"where\": {"
                        "\"fieldFilter\": {"
                        "\"field\": {\"fieldPath\": \"vehicle\"},"
                        "\"op\": \"EQUAL\","
                        "\"value\": {\"stringValue\": \"" + vehicle_id + "\"}"
                        "}}}}";

    String url = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents:runQuery";
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(queryBody);
    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
    }
    http.end();
}

// 🔹 Function to Update Payment Status to Inactive
void updatePaymentStatusToInactive(String lastVehicleID) {
    Serial.println("Updating payment status for Vehicle ID: " + lastVehicleID);
}
