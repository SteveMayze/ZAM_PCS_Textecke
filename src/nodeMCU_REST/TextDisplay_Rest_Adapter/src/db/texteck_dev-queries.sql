

SELECT
    id,
    event_ts, -- Displays in your session's local time (CET)
    systimestamp,
    EXTRACT(DAY FROM (systimestamp - event_ts)) * 1440 +
    EXTRACT(HOUR FROM (systimestamp - event_ts)) * 60 +
    EXTRACT(MINUTE FROM (systimestamp - event_ts)) AS minutes,
    event_name,
    message,
    length(message) as message_length,
    ip_address
FROM events
ORDER BY event_ts DESC;

truncate table events;
delete events where id < 100;
commit;


