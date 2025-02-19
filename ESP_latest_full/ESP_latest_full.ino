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

}

}

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
    extractedData.trim();  // Remove leading/trailing whitespace
    Serial.println("Extracted Data: " + extractedData);

    // Ensure at least one valid data field
    if (extractedData.length() == 0) {
        Serial.println("Error: Empty data");
        return false;
    }

    int index = 0, start = 0;
    int length = extractedData.length();

    for (int i = 0; i < length; i++) {
        if (extractedData[i] == ',' || i == length - 1) {
            if (i == length - 1) i++;  // Include last character if no trailing comma

            // Extract value safely
            String value = extractedData.substring(start, i);
            value.trim();  // Remove spaces within values

            // Store value only if within bounds
            if (index < 20) {
                values[index++] = value;
            } else {
                Serial.println("Warning: Too many fields, extra data ignored.");
                break;
            }
            start = i + 1;
        }
    }

    // Ensure correct number of fields
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
                  "\"MAX_PRESS\": {\"doubleValue\": " + String(values[1].toFloat(), 2) + "},"
                  "\"MAX_TEMP\": {\"doubleValue\": " + String(values[2].toFloat(), 2) + "},"
                  "\"TANK_PRESSURE\": {\"doubleValue\": " + String(values[3].toFloat(), 2) + "},"
                  "\"TANK_TEMP\": {\"doubleValue\": " + String(values[4].toFloat(), 2) + "},"
                  "\"CURRENT_GAS\": {\"doubleValue\": " + String(values[5].toFloat(), 2) + "},"
                  "\"SOC\": {\"doubleValue\": " + String(values[6].toFloat(), 2) + "},"
                  "\"STATUS\": {\"stringValue\": \"" + values[7] + "\"},"
                  "\"MAX_DELIVERY_PRESS\": {\"doubleValue\": " + String(values[9].toFloat(), 2) + "},"
                  "\"PRECOOL_TEMP\": {\"doubleValue\": " + String(values[10].toFloat(), 2) + "},"
                  "\"FLOW_RATE_LIMIT\": {\"doubleValue\": " + String(values[11].toFloat(), 2) + "},"
                  "\"FLOW_RATE\": {\"doubleValue\": " + String(values[12].toFloat(), 2) + "},"
                  "\"DELIVERY_PRESS\": {\"doubleValue\": " + String(values[13].toFloat(), 2) + "},"
                  "\"DELIVERY_TEMP\": {\"doubleValue\": " + String(values[14].toFloat(), 2) + "},"
                  "\"station_id\": {\"stringValue\": \"Stesen Jalan Pudu\"},"
                  "\"dispenser_type\": {\"stringValue\": \"H20\"},"
                  "\"location\": {\"stringValue\": \"Pudu\"}"
                  "}}";



    sendFirestoreRequest(stationURL, jsonData);
}

// 🔹 Function to Find and Update Payment Document
void findAndUpdatePayment(String vehicle_id, String values[]) {
    // Construct the Firestore query body in JSON format to filter by vehicle_id
    String queryBody = "{"
                        "\"structuredQuery\": {"
                        "\"select\": {\"fields\": [{\"fieldPath\": \"vehicle_id\"}]},"   // Select the vehicle_id field
                        "\"from\": [{\"collectionId\": \"payment\", \"allDescendants\": false}],"
                        "\"where\": {"
                        "\"fieldFilter\": {"
                        "\"field\": {\"fieldPath\": \"vehicle_id\"},"
                        "\"op\": \"EQUAL\","
                        "\"value\": {\"stringValue\": \"" + vehicle_id + "\"}"
                        "}"
                        "}"
                        "}"
                        "}";

    String url = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents:runQuery";

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Send the query request with the structured query body
    int httpCode = http.POST(queryBody);
    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("Full Firestore Response: " + response);  // Print full response for debugging

        // Extract document ID from the response (assuming response contains the document info)
        String docID = extractDocumentID(response);
        if (docID != "") {
            Serial.println("Found document ID: " + docID);  // Debugging found document ID
            updatePaymentDocument(docID, values);  // Update the payment document with the ID
        } else {
            Serial.println("No matching payment document found.");
        }
    } else {
        Serial.println("Error finding payment: " + String(httpCode));
    }
    http.end();
}


// 🔹 Function to Extract Document ID from Firestore Response
String extractDocumentID(String response) {
    int startIndex = response.indexOf("\"name\": \"projects/ihax-738e6/databases/(default)/documents/payment/") + 67;
    if (startIndex > 67) {
        int endIndex = response.indexOf("\"", startIndex);
        return response.substring(startIndex, endIndex);
    }
    return "";
}

// 🔹 Function to Update Payment Document
void updatePaymentDocument(String docID, String values[]) {
    String url = paymentURL + "/" + docID;


    String jsonData = "{\"fields\": {"
                      "\"vehicle_id\": {\"stringValue\": \"" + values[8] + "\"},"
                      "\"status\": {\"stringValue\": \"Active\"},"
                      "\"activeat\": {\"stringValue\": \"" + values[15] + "\"}"
                      "}}";

    sendFirestoreRequest(url, jsonData);
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

// 🔹 Function to Update Payment Status to Inactive
void updatePaymentStatusToInactive(String lastVehicleID) {
    String queryBody = "{"
                        "\"structuredQuery\": {"
                        "\"select\": {\"fields\": [{\"fieldPath\": \"vehicle_id\"}]},"   
                        "\"from\": [{\"collectionId\": \"payment\", \"allDescendants\": false}],"
                        "\"where\": {"
                        "\"fieldFilter\": {"
                        "\"field\": {\"fieldPath\": \"vehicle_id\"},"
                        "\"op\": \"EQUAL\","
                        "\"value\": {\"stringValue\": \"" + lastVehicleID + "\"}"
                        "}"
                        "}"
                        "}"
                        "}";

    String url = "https://firestore.googleapis.com/v1/projects/ihax-738e6/databases/(default)/documents:runQuery";

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(queryBody);
    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("Inactive Update Response: " + response);  

        String docID = extractDocumentID(response);
        if (docID != "") {
            Serial.println("Updating last vehicle (" + lastVehicleID + ") to Inactive.");
            updatePaymentDocumentInactive(docID, lastVehicleID);
        } else {
            Serial.println("No matching document found to update.");
        }
    } else {
        Serial.println("Error updating inactive status: " + String(httpCode));
    }
    http.end();
}

// 🔹 Function to Send "Inactive" Update
void updatePaymentDocumentInactive(String docID, String lastVehicleID) {
    String url = paymentURL + "/" + docID;

    String jsonData = "{\"fields\": {"
                      "\"status\": {\"stringValue\": \"Inactive\"},"
                      "\"activeat\": {\"stringValue\": \"\"},"
                      "\"vehicle_id\": {\"stringValue\": \"" + lastVehicleID + "\"}"
                      "}}";

    sendFirestoreRequest(url, jsonData);
}

