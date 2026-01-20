# Database Logging Integration Guide

## Overview
This guide shows how to integrate the `DatabaseLogger` class into your TextDisplay_Rest_Adapter to log API events to an Oracle Cloud database via ORDS REST endpoint.

## Files Created
1. **include/database_logger.h** - Header file with DatabaseLogger class definition
2. **src/database_logger.cpp** - Implementation of database logging functionality

## Integration Steps

### 1. Update main.cpp Header Includes
Add this include at the top of your `src/main.cpp`:

```cpp
#include "database_logger.h"
```

### 2. Initialize in setup()
In your `setup()` function, after WiFi connects and the server starts, call:

```cpp
void setup() {
    // ... existing code ...
    
    setup_rest_api();
    DatabaseLogger::begin();  // Add this line
    
    // ... rest of setup ...
}
```

### 3. Log Message Events
In the `/api/v1/message` POST handler, add logging after successfully processing:

```cpp
// In the /api/v1/message POST handler body callback
if (index + len == total) {
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, buf.c_str(), buf.size());
    if (err) {
        request->send(400, "application/json; charset=utf-8", "{\"error\":\"Invalid JSON\"}");
    } else {
        // ... validation code ...
        if (txt_len <= 200) {
            JsonVariant var = doc.as<JsonVariant>();
            handle_rest_request(request, var);
            
            // Log the event to database
            DatabaseLogger::logMessageEvent(txt);  // Add this line
        }
    }
    postBodyBuffer.erase(request);
}
```

### 4. Log Color Events
In the `/api/v1/color` POST handler, add logging after successfully applying colors:

```cpp
// In the /api/v1/color POST handler body callback
if (index + len == total) {
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, buf.c_str(), buf.size());
    if (err) {
        request->send(400, "application/json; charset=utf-8", "{\"error\":\"Invalid JSON\"}");
    } else {
        // ... validation code ...
        if (hasFg || hasBg) {
            // ... color assignment code ...
            
            // Apply colors to display
            String tempstr = String(foreground);
            tempstr.concat(":");
            tempstr.concat(background);
            render_and_send("color", tempstr.c_str());
            
            // Log the event to database
            DatabaseLogger::logColorEvent(foreground, background);  // Add this line
            
            request->send(200, "application/json; charset=utf-8", "{\"status\":\"ok\"}");
        }
    }
    postBodyBuffer.erase(request);
}
```

## How It Works

### Architecture
The `DatabaseLogger` class sends HTTP POST requests to your Oracle ORDS endpoint with JSON payloads containing event information.

### Request Format
```json
{
    "event_type": "MESSAGE",           // or "COLOR"
    "event_data": "{\"text\":\"...\"}",
    "timestamp": 12345678
}
```

### Features
- **Automatic Retries**: Attempts up to 2 retries on failure
- **Error Handling**: Logs all errors with detailed messages
- **JSON Serialization**: Uses ArduinoJson for consistent data formatting
- **Timeout Protection**: 5-second timeout to prevent hanging
- **Cross-Platform**: Works on both ESP32 and ESP8266

### Platform Support
- **ESP32**: Uses WiFi.h and HTTPClient
- **ESP8266**: Uses ESP8266WiFi.h and ESP8266HTTPClient

## Oracle ORDS Configuration

The logger expects your Oracle database to have a REST endpoint at:
```
https://b0o5r0vxkz5ekbo-db202011271753.adb.eu-frankfurt-1.oraclecloudapps.com/ords/texteck_dev/events
```

The endpoint should accept POST requests with the following body:

```json
{
    "event_type": "string",
    "event_data": "string (JSON format)",
    "timestamp": "number (milliseconds)"
}
```

### Optional: Create Oracle Table
If you need to create the table in Oracle:

```sql
CREATE TABLE events (
    event_id NUMBER GENERATED ALWAYS AS IDENTITY,
    event_type VARCHAR2(50),
    event_data CLOB,
    created_at TIMESTAMP DEFAULT SYSTIMESTAMP,
    esp32_timestamp NUMBER,
    PRIMARY KEY (event_id)
);
```

### Optional: Create ORDS Handler
Create a simple ORDS handler that inserts the data:

```sql
BEGIN
    INSERT INTO events (event_type, event_data, esp32_timestamp)
    VALUES (
        :event_type,
        :event_data,
        :timestamp
    );
    COMMIT;
END;
/
```

## Usage Examples

### Basic Logging
```cpp
// Log a message
DatabaseLogger::logMessageEvent("Welcome to ZAM");

// Log colors
DatabaseLogger::logColorEvent("white", "black");

// Log custom event
DatabaseLogger::logCustomEvent("STARTUP", "{\"version\":\"1.0\"}");
```

### With Error Handling
```cpp
if (DatabaseLogger::logMessageEvent(txt)) {
    LOG_INFO_LN("Event logged successfully");
} else {
    LOG_ERROR_LN("Failed to log event");
}
```

## Memory Considerations
- Stack-allocated JSON documents use up to 512 bytes per request
- HTTPClient handles memory internally
- Should work fine on ESP32 with typical RAM available

## Performance
- Non-blocking HTTP requests take approximately 100-500ms (depending on network)
- Consider logging asynchronously if response time is critical
- Max 2 retry attempts prevent excessive delays

## Troubleshooting

### No Events Logged
1. Verify WiFi connection is active before sending requests
2. Check that the Oracle endpoint is accessible from your network
3. Verify CORS headers if needed
4. Enable debug logging to see HTTP response codes

### Connection Errors
1. Verify SSL certificate validity (ESP32 may reject self-signed certs)
2. Check firewall rules allow outbound HTTPS
3. Increase TIMEOUT_MS if network is slow
4. Use `LOG_DEBUG_F()` to see detailed error messages

### Database Not Receiving Data
1. Verify the endpoint accepts POST requests
2. Check that event_type and event_data columns exist
3. Review Oracle database logs for insert errors
4. Verify authentication if required (add HTTP headers as needed)

## Future Enhancements

1. **Batching**: Queue events and send in batches to reduce network overhead
2. **Local Queue**: Store failed events locally and retry later
3. **Compression**: Compress JSON payloads for efficiency
4. **Authentication**: Add API key or OAuth token support
5. **Real Time**: Integrate NTP for accurate timestamps
6. **Async Logging**: Use task queues to prevent blocking

