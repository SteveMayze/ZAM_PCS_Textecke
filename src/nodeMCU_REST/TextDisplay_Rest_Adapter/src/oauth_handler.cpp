#include "oauth_handler.h"
#include "logger.h"
#include "secrets.h"

// Initialize static members
char OAuthHandler::accessToken[OAuthHandler::TOKEN_BUFFER_SIZE] = "";
unsigned long OAuthHandler::tokenExpiryTime = 0;
bool OAuthHandler::tokenValid = false;

// OAuth credentials from secrets.h
const char* OAuthHandler::OAUTH_URL = OAUTH_TOKEN_URL;
const char* OAuthHandler::OAUTH_CLIENT_ID = OAUTH_CLIENT_ID_SECRET;
const char* OAuthHandler::OAUTH_CLIENT_SECRET = OAUTH_CLIENT_SECRET_SECRET;

void OAuthHandler::begin() {
    LOG_INFO_LN("OAuthHandler initialized");
    // Token will be requested on first getAccessToken() call
}

String OAuthHandler::generateBase64Credentials() {
    // Create credentials string in format: client_id:client_secret
    String credentials = String(OAUTH_CLIENT_ID);
    credentials.concat(":");
    credentials.concat(OAUTH_CLIENT_SECRET);
    
    LOG_DEBUG_F("OAuthHandler: Credentials string length: %d\n", credentials.length());
    
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
            
            for (int j = 0; j < 4; j++) {
                encoded += base64_chars[char_array_4[j]];
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
    
    LOG_DEBUG_F("OAuthHandler: Base64 encoded credentials length: %d\n", encoded.length());
    return encoded;
}

bool OAuthHandler::requestNewToken() {
    LOG_INFO_LN("OAuthHandler: Requesting new access token...");
    
    #if defined(TEXTECKE_ESP32)
        HTTPClient http;
    #elif defined(TEXTECKE_ESP8266)
        HTTPClient http;
    #else
        LOG_ERROR_LN("OAuthHandler: Unsupported platform");
        return false;
    #endif
    
    http.begin(OAUTH_URL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setTimeout(TIMEOUT_MS);
    
    // Create Authorization header with base64 encoded credentials
    String basicAuth = "Basic ";
    basicAuth.concat(generateBase64Credentials());
    http.addHeader("Authorization", basicAuth);
    
    // OAuth 2.0 Client Credentials flow body
    String payload = "grant_type=client_credentials";
    
    LOG_DEBUG_F("OAuthHandler: Requesting token from: %s\n", OAUTH_URL);
    LOG_DEBUG_F("OAuthHandler: Authorization header format: Basic [base64 encoded]\n");
    LOG_DEBUG_F("OAuthHandler: Client ID length: %d\n", strlen(OAUTH_CLIENT_ID));
    LOG_DEBUG_F("OAuthHandler: Client Secret length: %d\n", strlen(OAUTH_CLIENT_SECRET));
    LOG_DEBUG_F("OAuthHandler: Payload: %s\n", payload.c_str());
    
    int httpResponseCode = http.POST(payload);
    
    LOG_DEBUG_F("OAuthHandler: HTTP Response Code: %d\n", httpResponseCode);
    
    if (httpResponseCode == 200) {
        LOG_INFO_F("OAuthHandler: Token request successful (HTTP %d)\n", httpResponseCode);
        
        String response = http.getString();
        LOG_DEBUG_F("OAuthHandler: Response length: %d\n", response.length());
        LOG_DEBUG_F("OAuthHandler: Response: %s\n", response.c_str());
        
        // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            LOG_ERROR_F("OAuthHandler: JSON parse error: %s\n", error.f_str());
            http.end();
            return false;
        }
        
        // Extract access token
        if (!doc.containsKey("access_token")) {
            LOG_ERROR_LN("OAuthHandler: No access_token in response");
            http.end();
            return false;
        }
        
        const char* token = doc["access_token"];
        int expiresIn = doc["expires_in"] | 3600;  // Default to 1 hour if not specified
        
        LOG_DEBUG_F("OAuthHandler: Token expires in: %d seconds\n", expiresIn);
        
        // Store token and expiry time
        if (strlen(token) < TOKEN_BUFFER_SIZE) {
            strcpy(accessToken, token);
            tokenExpiryTime = millis() + (expiresIn * 1000);  // Convert to milliseconds
            tokenValid = true;
            
            LOG_INFO_F("OAuthHandler: Access token stored (length: %d)\n", strlen(token));
            http.end();
            return true;
        } else {
            LOG_ERROR_LN("OAuthHandler: Token too large for buffer");
            http.end();
            return false;
        }
    } else if (httpResponseCode > 0) {
        LOG_ERROR_F("OAuthHandler: HTTP error %d\n", httpResponseCode);
        String response = http.getString();
        LOG_ERROR_F("OAuthHandler: Response length: %d\n", response.length());
        if (response.length() > 0) {
            LOG_ERROR_F("OAuthHandler: Response: %s\n", response.c_str());
        } else {
            LOG_ERROR_LN("OAuthHandler: Empty response body");
        }
        
        // Log additional debug info
        LOG_DEBUG_F("OAuthHandler: Status code: %d (typically 401 = Unauthorized, 400 = Bad Request)\n", httpResponseCode);
        if (httpResponseCode == 401) {
            LOG_ERROR_LN("OAuthHandler: 401 Unauthorized - Check client ID, secret, and OAuth endpoint");
        } else if (httpResponseCode == 400) {
            LOG_ERROR_LN("OAuthHandler: 400 Bad Request - Check request format and credentials");
        }
    } else {
        LOG_ERROR_F("OAuthHandler: Connection error: %s\n", http.errorToString(httpResponseCode).c_str());
        LOG_ERROR_F("OAuthHandler: Error code: %d\n", httpResponseCode);
    }
    
    http.end();
    tokenValid = false;
    return false;
}

const char* OAuthHandler::getAccessToken() {
    // Check if we have a valid token that hasn't expired
    if (tokenValid && millis() < tokenExpiryTime) {
        LOG_DEBUG_LN("OAuthHandler: Using cached access token");
        return accessToken;
    }
    
    // Token is invalid or expired, request a new one
    if (requestNewToken()) {
        return accessToken;
    } else {
        LOG_ERROR_LN("OAuthHandler: Failed to obtain access token");
        return NULL;
    }
}

bool OAuthHandler::refreshToken() {
    LOG_INFO_LN("OAuthHandler: Forcing token refresh...");
    tokenValid = false;
    memset(accessToken, 0, TOKEN_BUFFER_SIZE);
    return requestNewToken();
}

bool OAuthHandler::isTokenValid() {
    return tokenValid && millis() < tokenExpiryTime;
}

long OAuthHandler::getTokenRemainingTime() {
    if (!tokenValid) {
        return 0;
    }
    
    long remaining = (long)tokenExpiryTime - (long)millis();
    return remaining / 1000;  // Convert milliseconds to seconds
}

void OAuthHandler::clearToken() {
    LOG_DEBUG_LN("OAuthHandler: Clearing cached token");
    tokenValid = false;
    memset(accessToken, 0, TOKEN_BUFFER_SIZE);
    tokenExpiryTime = 0;
}
