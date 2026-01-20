# OAuth 2.0 Implementation for ESP32 Oracle ORDS Integration

## Overview

This document describes the OAuth 2.0 implementation for authenticating ESP32 requests to Oracle Cloud ORDS REST API endpoints. The implementation replaces the previous Basic Authentication method with OAuth 2.0 Bearer tokens, providing better security and compliance with modern API standards.

## Architecture

### Components

1. **OAuthHandler** (`oauth_handler.h` / `oauth_handler.cpp`)
   - Manages OAuth 2.0 Client Credentials flow
   - Handles token acquisition and caching
   - Automatically refreshes expired tokens
   - Thread-safe token management with expiry tracking

2. **DatabaseLogger** (updated)
   - Uses OAuth tokens instead of Basic Auth
   - Automatically obtains tokens from OAuthHandler
   - Handles 401 (Unauthorized) responses by refreshing tokens
   - Maintains API compatibility

3. **Secrets Configuration** (updated `secrets.h`)
   - Stores OAuth URL
   - Stores Client ID and Client Secret
   - Kept secure - never commit real credentials to public repos

## OAuth 2.0 Client Credentials Flow

The implementation uses the **Client Credentials Grant** flow, which is ideal for server-to-server communication:

```
ESP32                          OAuth Server            ORDS API
  |                                 |                     |
  |----request token with---------->|                     |
  |   client_id:client_secret       |                     |
  |   (Basic Auth header)           |                     |
  |                                 |                     |
  |<------access_token + expires----                     |
  |                                 |                     |
  |                                 |                     |
  |--POST event with Bearer token----------------->    |
  |   (Authorization: Bearer <token>)                    |
  |                                 |                    |
  |                            [log event]              |
  |                                 |                    |
  |<------201 Created---------------|                   |
  |                                 |                    |
```

## Setup Instructions

### 1. Update Credentials

Edit [secrets.h](src/secrets.h) with your Oracle OAuth credentials:

```cpp
#define OAUTH_TOKEN_URL "https://your-instance.adb.region.oraclecloudapps.com/ords/your_schema/oauth/token"
#define OAUTH_CLIENT_ID_SECRET "your_client_id"
#define OAUTH_CLIENT_SECRET_SECRET "your_client_secret"
```

**Important**: Never commit real credentials. Use environment variables or pre-build configuration in production.

### 2. Initialization

The OAuth handler is initialized automatically in `DatabaseLogger::begin()`:

```cpp
void setup() {
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    DatabaseLogger::begin();  // Initializes both DatabaseLogger and OAuthHandler
}
```

### 3. Usage

Use the same API as before - authentication is now handled internally:

```cpp
// Log a message event
DatabaseLogger::logMessageEvent("Welcome to ZAM", client_ip);

// Log a color event
DatabaseLogger::logColorEvent("white", "black", client_ip);

// Log custom event
DatabaseLogger::logCustomEvent("CUSTOM_EVENT", "event_data", client_ip);
```

## Key Features

### Token Caching
- Tokens are cached in memory to avoid unnecessary API calls
- Cached tokens are reused until expiration
- Automatic refresh on next request when token expires
- Reduces latency and bandwidth usage

### Automatic Token Refresh
- Tokens are automatically refreshed when expired
- If API returns 401 (Unauthorized), token is forcibly refreshed
- Retries failed API calls after token refresh

### Memory Efficient
- Token buffer limited to 2KB (more than enough for OAuth tokens)
- Minimal overhead with static member variables
- Suitable for ESP32's memory constraints

### Error Handling
- Comprehensive logging at all stages
- Graceful degradation if OAuth fails
- Detailed error messages for debugging

## API Reference

### OAuthHandler::getAccessToken()
```cpp
const char* token = OAuthHandler::getAccessToken();
// Returns valid token or NULL if unable to obtain
// Automatically handles token refresh
```

### OAuthHandler::refreshToken()
```cpp
bool success = OAuthHandler::refreshToken();
// Forces immediate token refresh from server
// Useful if you suspect token compromise
```

### OAuthHandler::isTokenValid()
```cpp
if (OAuthHandler::isTokenValid()) {
    // Current cached token is valid
}
```

### OAuthHandler::getTokenRemainingTime()
```cpp
long remainingSeconds = OAuthHandler::getTokenRemainingTime();
LOG_INFO_F("Token expires in: %ld seconds\n", remainingSeconds);
```

### OAuthHandler::clearToken()
```cpp
OAuthHandler::clearToken();
// Manually clear cached token
// Useful for testing or token invalidation scenarios
```

## Building and Testing

### Build for ESP32
```bash
cd TextDisplay_Rest_Adapter
pio run -e texteck-esp32
```

### Serial Monitor
```bash
pio device monitor -e texteck-esp32
```

Expected debug output:
```
OAuthHandler initialized
OAuthHandler: Requesting new access token...
OAuthHandler: Token request successful (HTTP 200)
OAuthHandler: Access token stored (length: 256)
OAuthHandler: Token expires in: 3600 seconds
DatabaseLogger: Sending event to https://...
DatabaseLogger: Token remaining time: 3599 seconds
DatabaseLogger: Event logged successfully (HTTP 201)
```

## Security Considerations

### 1. Credential Storage
- Never hardcode credentials in released binaries
- Use secure storage mechanisms for production
- Consider using OTA updates with encrypted credentials

### 2. Token Handling
- Tokens are stored in RAM (volatile memory)
- Cleared on reset
- Never logged to persistent storage
- Transmitted only over HTTPS

### 3. Certificate Validation
- ESP32 HTTPClient verifies SSL certificates by default
- Ensures secure communication with OAuth server
- Prevents man-in-the-middle attacks

### 4. HTTPS Only
- All OAuth communication uses HTTPS
- API calls to ORDS also use HTTPS
- Credentials never transmitted over unencrypted channels

## Troubleshooting

### Issue: Token Request Failed
```
OAuthHandler: Token request error: Connection refused
```
**Solutions**:
- Verify WiFi connection
- Check OAuth URL is correct and reachable
- Verify client ID and secret are correct
- Check firewall/proxy settings

### Issue: 401 Unauthorized on API Call
```
DatabaseLogger: HTTP error 401
OAuthHandler: Received 401, token may be invalid. Refreshing token...
```
**Solutions**:
- Normal behavior - token refreshes automatically
- Check that ORDS API is configured to accept OAuth tokens
- Verify API endpoint permissions for OAuth user

### Issue: Token Too Large for Buffer
```
OAuthHandler: Token too large for buffer
```
**Solutions**:
- Increase `TOKEN_BUFFER_SIZE` in `oauth_handler.h`
- Contact Oracle support about token size

### Issue: Memory Running Low
- Reduce log level in `platformio.ini` build flags
- Consider disabling database logging with `ENABLE_DATABASE_LOGGING`
- Use more memory-efficient ORDS endpoints

## Switching Back to Basic Auth

To revert to Basic Authentication:

1. Edit [database_logger.cpp](src/database_logger.cpp)
2. Restore the `generateBasicAuthHeader()` function
3. Comment out `OAuthHandler::begin()` call
4. Replace OAuth token header with Basic Auth header
5. Add back `ORACLE_ORDS_USER` and `ORACLE_ORDS_PASS` to [secrets.h](src/secrets.h)

Note: OAuth is recommended for production deployments.

## References

- [OAuth 2.0 Authorization Framework](https://datatracker.ietf.org/doc/html/rfc6749)
- [OAuth 2.0 Client Credentials Grant](https://datatracker.ietf.org/doc/html/rfc6749#section-4.4)
- [Oracle ORDS OAuth Documentation](https://docs.oracle.com/cd/E56351_01/doc.30/e87809/developing_REST_API.htm)
- [ArduinoJson Documentation](https://arduinojson.org/)

## Support

For issues or questions:
1. Check the debug log output
2. Verify credentials in [secrets.h](src/secrets.h)
3. Test OAuth endpoint independently (e.g., with cURL)
4. Review error messages in the troubleshooting section
