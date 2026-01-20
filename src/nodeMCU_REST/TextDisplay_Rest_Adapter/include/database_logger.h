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
     * @param eventName The event name to discriminate resource type (MESSAGE, COLOR, ERROR, etc.)
     * @param message The message content to log
     * @param clientIP The IP address of the HTTP client/sender (optional, can be NULL)
     * @return true if successfully sent, false otherwise
     */
    static bool sendEventToOracle(const char* eventName, const char* message, const char* clientIP);
    
public:
    /**
     * @brief Initialize the database logger
     * Should be called after WiFi is connected
     */
    static void begin();
    
    /**
     * @brief Log a message event to the database
     * @param text The message text that was sent
     * @param clientIP The IP address of the HTTP client/sender (optional, can be NULL)
     * @return true if logged successfully
     */
    static bool logMessageEvent(const char* text, const char* clientIP = NULL);
    
    /**
     * @brief Log a color event to the database
     * @param foreground Foreground color
     * @param background Background color
     * @param clientIP The IP address of the HTTP client/sender (optional, can be NULL)
     * @return true if logged successfully
     */
    static bool logColorEvent(const char* foreground, const char* background, const char* clientIP = NULL);
    
    /**
     * @brief Log a raw event with custom event type and data
     * @param eventType Type of event
     * @param eventData Event data as JSON string
     * @param clientIP The IP address of the HTTP client/sender (optional, can be NULL)
     * @return true if logged successfully
     */
    static bool logCustomEvent(const char* eventType, const char* eventData, const char* clientIP = NULL);
};

#endif // DATABASE_LOGGER_H
