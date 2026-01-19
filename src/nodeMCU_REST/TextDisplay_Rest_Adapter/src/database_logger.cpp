#include "database_logger.h"
#include "logger.h"
#include "secrets.h"

// Oracle Cloud ORDS configuration
const char* DatabaseLogger::ORACLE_ENDPOINT = "https://b0o5r0vxkz5ekbo-db202011271753.adb.eu-frankfurt-1.oraclecloudapps.com";
const char* DatabaseLogger::EVENT_RESOURCE = "/ords/texteck_dev/events";
const char* DatabaseLogger::ORACLE_USERNAME = ORACLE_ORDS_USER;
const char* DatabaseLogger::ORACLE_PASSWORD = ORACLE_ORDS_PASS;

void DatabaseLogger::begin() {
    LOG_INFO_LN("DatabaseLogger initialized");
    #if defined(TEXTECKE_ESP32)
        // Optional: Configure SSL certificate verification if needed
        // HTTPClient accepts self-signed certificates by default on ESP32
    #endif
}

String DatabaseLogger::generateBasicAuthHeader() {
    // Create credentials string in format: username:password
    String credentials = String(ORACLE_USERNAME);
    credentials.concat(":");
    credentials.concat(ORACLE_PASSWORD);
    
    LOG_DEBUG_F("DatabaseLogger: Credentials length: %d\n", credentials.length());
    
    // Base64 encode the credentials
    // We'll use a simple Base64 encoding implementation
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    String encoded;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (size_t n = 0; n < credentials.length(); n++) {
        char_array_3[i++] = (unsigned char)credentials[n];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                encoded += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i > 0) {
        for (int j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j <= i; j++) {
            encoded += base64_chars[char_array_4[j]];
        }
        
        while (i++ < 3) {
            encoded += '=';
        }
    }
    
    LOG_DEBUG_F("DatabaseLogger: Base64 encoded credentials length: %d\n", encoded.length());
    return encoded;
}

bool DatabaseLogger::sendEventToOracle(const char* eventType, const char* eventData) {
    if (!eventType || !eventData) {
        LOG_ERROR_LN("DatabaseLogger: Invalid parameters");
        return false;
    }
    
    #if defined(TEXTECKE_ESP32)
        HTTPClient http;
    #elif defined(TEXTECKE_ESP8266)
        HTTPClient http;
    #else
        LOG_ERROR_LN("DatabaseLogger: Unsupported platform");
        return false;
    #endif
    
    // Construct full URL
    String url = String(ORACLE_ENDPOINT);
    url.concat(EVENT_RESOURCE);
    
    LOG_INFO_F("DatabaseLogger: Sending %s event to %s\n", eventType, url.c_str());
    
    bool success = false;
    int attempts = 0;
    
    while (attempts < MAX_RETRIES && !success) {
        attempts++;
        
        if (attempts > 1) {
            LOG_INFO_F("DatabaseLogger: Retry attempt %d\n", attempts);
            delay(500); // Wait before retry
        }
        
        // Create JSON payload
        DynamicJsonDocument doc(512);
        doc["event_type"] = eventType;
        doc["event_data"] = eventData;
        doc["timestamp"] = millis(); // You can enhance this with real time if NTP is available
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        
        LOG_DEBUG_F("DatabaseLogger: Payload: %s\n", jsonPayload.c_str());
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(TIMEOUT_MS);
        
        // Add Basic Authentication header
        String basicAuth = "Basic ";
        basicAuth.concat(generateBasicAuthHeader());
        http.addHeader("Authorization", basicAuth);
        
        LOG_DEBUG_F("DatabaseLogger: Authorization header set\n");
        
        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            LOG_INFO_F("DatabaseLogger: Event logged successfully (HTTP %d)\n", httpResponseCode);
            success = true;
        } else if (httpResponseCode > 0) {
            LOG_ERROR_F("DatabaseLogger: HTTP error %d\n", httpResponseCode);
            String response = http.getString();
            LOG_ERROR_F("DatabaseLogger: Response: %s\n", response.c_str());
        } else {
            LOG_ERROR_F("DatabaseLogger: Connection error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        
        http.end();
    }
    
    return success;
}

bool DatabaseLogger::logMessageEvent(const char* text) {
    if (!text) {
        LOG_ERROR_LN("DatabaseLogger: Message text is null");
        return false;
    }
    
    DynamicJsonDocument doc(256);
    doc["text"] = text;
    doc["length"] = strlen(text);
    
    String eventData;
    serializeJson(doc, eventData);
    
    return sendEventToOracle("MESSAGE", eventData.c_str());
}

bool DatabaseLogger::logColorEvent(const char* foreground, const char* background) {
    if (!foreground || !background) {
        LOG_ERROR_LN("DatabaseLogger: Color parameters are null");
        return false;
    }
    
    DynamicJsonDocument doc(256);
    doc["fg"] = foreground;
    doc["bg"] = background;
    
    String eventData;
    serializeJson(doc, eventData);
    
    return sendEventToOracle("COLOR", eventData.c_str());
}

bool DatabaseLogger::logCustomEvent(const char* eventType, const char* eventData) {
    if (!eventType || !eventData) {
        LOG_ERROR_LN("DatabaseLogger: Invalid event parameters");
        return false;
    }
    
    return sendEventToOracle(eventType, eventData);
}
