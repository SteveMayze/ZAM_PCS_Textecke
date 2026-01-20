#include "database_logger.h"
#include "logger.h"
#include "secrets.h"

#if !defined(USE_OAUTH) || USE_OAUTH
    #include "oauth_handler.h"
#endif

// Oracle Cloud ORDS configuration
const char* DatabaseLogger::ORACLE_ENDPOINT = ORACLE_ORDS_ENDPOINT;
const char* DatabaseLogger::EVENT_RESOURCE = ORACLE_ORDS_EVENT_RESOURCE;

#if defined(USE_OAUTH) && USE_OAUTH == 0
    // Basic Auth mode
    const char* DatabaseLogger::ORACLE_USERNAME = ORACLE_ORDS_USER;
    const char* DatabaseLogger::ORACLE_PASSWORD = ORACLE_ORDS_PASS;
#endif

void DatabaseLogger::begin() {
    LOG_INFO_LN("DatabaseLogger initialized");
    #if !defined(USE_OAUTH) || USE_OAUTH
        // OAuth mode
        OAuthHandler::begin();
        LOG_INFO_LN("Using OAuth 2.0 Bearer Token authentication");
    #else
        // Basic Auth mode
        LOG_INFO_LN("Using Basic Authentication");
    #endif
    
    #if defined(TEXTECKE_ESP32)
        // Optional: Configure SSL certificate verification if needed
        // HTTPClient accepts self-signed certificates by default on ESP32
    #endif
}


bool DatabaseLogger::sendEventToOracle(const char* eventName, const char* message, const char* clientIP) {
    if (!eventName || !message) {
        LOG_ERROR_LN("DatabaseLogger: Invalid eventName or message parameter");
        return false;
    }
    
    #if !defined(USE_OAUTH) || USE_OAUTH
        // OAuth mode - Get OAuth access token
        const char* token = OAuthHandler::getAccessToken();
        if (!token) {
            LOG_ERROR_LN("DatabaseLogger: Failed to obtain OAuth access token");
            return false;
        }
    #endif
    
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
    
    LOG_INFO_F("DatabaseLogger: Sending event to %s\n", url.c_str());
    LOG_DEBUG_F("DatabaseLogger: Full URL length: %d\n", url.length());
    
    #if !defined(USE_OAUTH) || USE_OAUTH
        LOG_DEBUG_F("DatabaseLogger: Token remaining time: %ld seconds\n", OAuthHandler::getTokenRemainingTime());
    #endif
    
    bool success = false;
    int attempts = 0;
    
    while (attempts < MAX_RETRIES && !success) {
        attempts++;
        
        if (attempts > 1) {
            LOG_INFO_F("DatabaseLogger: Retry attempt %d\n", attempts);
            delay(500); // Wait before retry
        }
        
        // Create JSON payload matching API specification
        // {
        //   "event_name": "string",
        //   "ip_address": "string",
        //   "message": "string"
        // }
        // Note: event_ts and id are handled by the ORDS API/database
        JsonDocument doc;
        
        // Event name to discriminate resource type (MESSAGE, COLOR, ERROR, etc.)
        doc["event_name"] = eventName;
        
        // IP address of the client (sender of the request)
        if (clientIP) {
            doc["ip_address"] = clientIP;
        } else {
            doc["ip_address"] = "unknown";
        }
        
        // Message content
        doc["message"] = message;
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        
        LOG_DEBUG_F("DatabaseLogger: Payload: %s\n", jsonPayload.c_str());
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(TIMEOUT_MS);
        
        // Configure proxy if enabled
        #ifdef USE_PROXY
            http.setConnectTimeout(TIMEOUT_MS);
            #if defined(TEXTECKE_ESP32)
                http.setProxy(IPAddress(192, 168, 1, 1)); // Will be replaced by PROXY_HOST
                http.setProxy(PROXY_HOST, PROXY_PORT);
            #elif defined(TEXTECKE_ESP8266)
                http.setProxy(PROXY_HOST, PROXY_PORT);
            #endif
            LOG_DEBUG_F("DatabaseLogger: Proxy configured - %s:%d\n", PROXY_HOST, PROXY_PORT);
        #endif
        
        // Add authentication header (OAuth or Basic Auth based on configuration)
        #if !defined(USE_OAUTH) || USE_OAUTH
            // OAuth Bearer Token authentication
            String bearerAuth = "Bearer ";
            bearerAuth.concat(token);
            http.addHeader("Authorization", bearerAuth);
            LOG_DEBUG_F("DatabaseLogger: Authorization header set (OAuth Bearer Token)\n");
        #else
            // Basic Authentication
            String basicAuth = "Basic ";
            basicAuth.concat(generateBasicAuthHeader());
            http.addHeader("Authorization", basicAuth);
            LOG_DEBUG_F("DatabaseLogger: Authorization header set (Basic Auth)\n");
        #endif
        
        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            LOG_INFO_F("DatabaseLogger: Event logged successfully (HTTP %d)\n", httpResponseCode);
            success = true;
        } else if (httpResponseCode > 0) {
            LOG_ERROR_F("DatabaseLogger: HTTP error %d\n", httpResponseCode);
            String response = http.getString();
            LOG_ERROR_F("DatabaseLogger: Response: %s\n", response.c_str());
            
            #if !defined(USE_OAUTH) || USE_OAUTH
                // If we get 401 (Unauthorized), token might be invalid, force refresh
                if (httpResponseCode == 401) {
                    LOG_INFO_LN("DatabaseLogger: Received 401, token may be invalid. Refreshing token...");
                    OAuthHandler::refreshToken();
                }
            #endif
        } else {
            LOG_ERROR_F("DatabaseLogger: Connection error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        
        http.end();
    }
    
    return success;
}

#if defined(USE_OAUTH) && USE_OAUTH == 0
    // Basic Auth mode only - include generateBasicAuthHeader implementation
    String DatabaseLogger::generateBasicAuthHeader() {
        // Create credentials string in format: username:password
        String credentials = String(ORACLE_USERNAME);
        credentials.concat(":");
        credentials.concat(ORACLE_PASSWORD);
        
        LOG_DEBUG_F("DatabaseLogger: Credentials length: %d\n", credentials.length());
        
        // Base64 encode the credentials
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
#endif

bool DatabaseLogger::logMessageEvent(const char* text, const char* clientIP) {
    if (!text) {
        LOG_ERROR_LN("DatabaseLogger: Message text is null");
        return false;
    }
    
    return sendEventToOracle("MESSAGE", text, clientIP);
}

bool DatabaseLogger::logColorEvent(const char* foreground, const char* background, const char* clientIP) {
    if (!foreground || !background) {
        LOG_ERROR_LN("DatabaseLogger: Color parameters are null");
        return false;
    }
    
    // Format both colors in the message: fg=<foreground>, bg=<background>
    String message = "fg=";
    message.concat(foreground);
    message.concat(", bg=");
    message.concat(background);
    
    return sendEventToOracle("COLOR", message.c_str(), clientIP);
}

bool DatabaseLogger::logCustomEvent(const char* eventType, const char* eventData, const char* clientIP) {
    if (!eventType || !eventData) {
        LOG_ERROR_LN("DatabaseLogger: Invalid event parameters");
        return false;
    }
    
    return sendEventToOracle(eventType, eventData, clientIP);
}
