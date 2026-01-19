#ifndef DATABASE_LOGGER_H
#define DATABASE_LOGGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(TEXTECKE_ESP32)
  #include <HTTPClient.h>
#elif defined(TEXTECKE_ESP8266)
  #include <ESP8266HTTPClient.h>
#endif

/**
 * @brief Database logging to Oracle Cloud via ORDS REST API
 * 
 * This module provides functionality to log message and color events
 * to an Oracle Cloud database using the ORDS REST endpoint.
 * Supports Basic Authentication via HTTP Authorization header.
 */

class DatabaseLogger {
private:
    static const char* ORACLE_ENDPOINT;
    static const char* EVENT_RESOURCE;
    static const int TIMEOUT_MS = 5000;
    static const int MAX_RETRIES = 2;
    static const char* ORACLE_USERNAME;
    static const char* ORACLE_PASSWORD;
    
    /**
     * @brief Generate Base64 encoded credentials for Basic Auth
     * Format: Base64(username:password)
     * @return String containing the Base64 encoded credentials
     */
    static String generateBasicAuthHeader();
    
    /**
     * @brief Send HTTP POST request to the Oracle ORDS endpoint
     * @param eventType Type of event ("MESSAGE" or "COLOR")
     * @param eventData The event data (message text or color values)
     * @return true if successfully sent, false otherwise
     */
    static bool sendEventToOracle(const char* eventType, const char* eventData);
    
public:
    /**
     * @brief Initialize the database logger
     * Should be called after WiFi is connected
     */
    static void begin();
    
    /**
     * @brief Log a message event to the database
     * @param text The message text that was sent
     * @return true if logged successfully
     */
    static bool logMessageEvent(const char* text);
    
    /**
     * @brief Log a color event to the database
     * @param foreground Foreground color
     * @param background Background color
     * @return true if logged successfully
     */
    static bool logColorEvent(const char* foreground, const char* background);
    
    /**
     * @brief Log a raw event with custom event type and data
     * @param eventType Type of event
     * @param eventData Event data as JSON string
     * @return true if logged successfully
     */
    static bool logCustomEvent(const char* eventType, const char* eventData);
};

#endif // DATABASE_LOGGER_H
