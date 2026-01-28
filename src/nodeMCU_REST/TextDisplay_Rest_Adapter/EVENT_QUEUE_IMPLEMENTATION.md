# Event Queue Implementation for Database Logging

## Problem Statement
After certificate expiration, the application was experiencing watchdog timer resets. The root cause was identified as database logging operations being performed synchronously within the async web server request handlers, which:
- Block the async request callback execution
- Violate the timing constraints of interrupt/task contexts
- Trigger watchdog timeouts due to lengthy I/O operations

## Solution Overview
Implemented an event-based queuing system that **decouples database operations from request handlers**. Events are now queued quickly in request handlers and processed asynchronously in the `loop()` function.

## Architecture Changes

### 1. Event Queue Data Structures
Added to `main.cpp`:

```cpp
enum EventType {
  EVENT_NONE,
  EVENT_MESSAGE_LOG,
  EVENT_COLOR_LOG
};

struct DatabaseEvent {
  EventType type;
  char message[MESSAGE_BUFFER_SIZE];
  char foreground_color[16];
  char background_color[16];
  char client_ip[16];
  unsigned long timestamp;
};

#define EVENT_QUEUE_SIZE 10
DatabaseEvent eventQueue[EVENT_QUEUE_SIZE];
uint8_t eventQueueHead = 0;
uint8_t eventQueueTail = 0;
uint8_t eventQueueCount = 0;
```

### 2. Queue Management Functions

**`queueDatabaseEvent(const DatabaseEvent &event)`**
- Enqueues an event with circular buffer logic
- Returns `false` if queue is full (logs warning)
- O(1) operation - minimal blocking in request handler

**`dequeueDatabaseEvent()`**
- Dequeues and returns next event
- Returns event with `type = EVENT_NONE` if queue is empty
- O(1) operation

**`processPendingDatabaseEvents()`**
- Called from `loop()`
- Processes all queued events sequentially
- Handles `EVENT_MESSAGE_LOG` and `EVENT_COLOR_LOG` types
- **No time constraints** - can block as needed for database I/O

### 3. Modified Request Handlers

All three database logging points were updated:

#### `handle_post_form_request()`
- **Before**: Called `DatabaseLogger::logMessageEvent()` and `DatabaseLogger::logColorEvent()` directly
- **After**: Queues `DatabaseEvent` structures for both operations

#### `handle_rest_request()`
- **Before**: Called `DatabaseLogger::logMessageEvent()` for text messages
- **After**: Queues `EVENT_MESSAGE_LOG` event
- **Before**: Called `DatabaseLogger::logColorEvent()` for color changes
- **After**: Queues `EVENT_COLOR_LOG` event (multiple locations)

#### Event Queueing Pattern
```cpp
DatabaseEvent msgEvent = {EVENT_MESSAGE_LOG, "", "", "", "", millis()};
strncpy(msgEvent.message, current_message, sizeof(msgEvent.message) - 1);
msgEvent.message[sizeof(msgEvent.message) - 1] = '\0';
strncpy(msgEvent.client_ip, clientIP.c_str(), sizeof(msgEvent.client_ip) - 1);
msgEvent.client_ip[sizeof(msgEvent.client_ip) - 1] = '\0';
queueDatabaseEvent(msgEvent);
```

### 4. Loop Function Implementation

Updated `loop()` to process pending events:

```cpp
void loop()
{
  // Process any pending database events without blocking
  processPendingDatabaseEvents();
  
  // Yield to other tasks and prevent watchdog timeout
  delay(10);
}
```

## Benefits

1. **Eliminates Watchdog Timeout**: Database I/O no longer blocks async callbacks
2. **Predictable Request Response**: Request handlers complete quickly
3. **Queue Safety**: Circular buffer with count tracking prevents race conditions
4. **Graceful Degradation**: Old events are dropped if queue fills (logged as warning)
5. **Non-Intrusive**: Minimal changes to existing logic, only decoupling the timing

## Constraints & Considerations

- **Queue Size**: Set to 10 events. Adjust `#define EVENT_QUEUE_SIZE` if needed
- **IP Address Buffer**: Limited to 16 bytes (IPv4 + null terminator)
- **Message Truncation**: Already handled in request handlers, preserved behavior
- **Processing Order**: Events processed in FIFO order, maintaining event causality
- **Frequency**: With typical usage patterns, the queue processes faster than events arrive

## Testing Recommendations

1. **Load Testing**: Send rapid requests and verify all events are logged
2. **Watchdog Stability**: Run for extended periods after certificate renewal
3. **Queue Overflow**: Monitor logs for "Event queue full" messages
4. **Database Latency**: Simulate slow database connections to verify no handler blocking

## Future Enhancements

- Make event queue size configurable via build flags
- Add telemetry for queue depth monitoring
- Implement configurable drop strategy (oldest vs. newest event)
- Consider separate queue processing thread (if using RTOS)
