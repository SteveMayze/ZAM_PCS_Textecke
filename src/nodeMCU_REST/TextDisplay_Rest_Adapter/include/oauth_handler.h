#ifndef OAUTH_HANDLER_H
#define OAUTH_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(TEXTECKE_ESP32)
  #include <HTTPClient.h>
#elif defined(TEXTECKE_ESP8266)
  #include <ESP8266HTTPClient.h>
#endif

/**
 * @brief OAuth 2.0 handler for Oracle Cloud ORDS
 * 
 * Implements OAuth 2.0 Client Credentials flow to obtain and manage access tokens
 * for authenticating with Oracle Cloud ORDS REST API endpoints.
 */
class OAuthHandler {
private:
    static const int TIMEOUT_MS = 5000;
    static const int TOKEN_BUFFER_SIZE = 2048;
    
    // OAuth credentials
    static const char* OAUTH_URL;
    static const char* OAUTH_CLIENT_ID;
    static const char* OAUTH_CLIENT_SECRET;
    
    // Token storage
    static char accessToken[TOKEN_BUFFER_SIZE];
    static unsigned long tokenExpiryTime;  // Unix timestamp when token expires
    static bool tokenValid;
    
    /**
     * @brief Generate Base64 encoded Client ID:Secret for OAuth authorization
     * @return String containing Base64 encoded credentials
     */
    static String generateBase64Credentials();
    
    /**
     * @brief Make an OAuth token request to the server
     * @return true if token successfully obtained, false otherwise
     */
    static bool requestNewToken();
    
public:
    /**
     * @brief Initialize the OAuth handler
     * Should be called after WiFi is connected
     */
    static void begin();
    
    /**
     * @brief Get a valid access token
     * If current token is expired or invalid, automatically requests a new one
     * @return Pointer to access token string, or NULL if token cannot be obtained
     */
    static const char* getAccessToken();
    
    /**
     * @brief Force refresh of access token
     * Useful if you want to ensure you have a fresh token
     * @return true if token successfully refreshed, false otherwise
     */
    static bool refreshToken();
    
    /**
     * @brief Check if current token is still valid
     * @return true if token exists and hasn't expired, false otherwise
     */
    static bool isTokenValid();
    
    /**
     * @brief Get remaining validity time of current token in seconds
     * @return Number of seconds token is still valid, 0 or negative if expired/invalid
     */
    static long getTokenRemainingTime();
    
    /**
     * @brief Clear cached token (forces refresh on next getAccessToken call)
     */
    static void clearToken();
};

#endif // OAUTH_HANDLER_H
