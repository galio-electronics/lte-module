#ifndef STATION_LTE_CONFIG_H
#define STATION_LTE_CONFIG_H
/* Edit before building. Host only: no tcp:// prefix or path. */
#define LTE_APN "internet"
#define LTE_MQTT_HOST "broker.example.com"
#define LTE_MQTT_PORT 1883
#define LTE_MQTT_CLIENT_ID "protolink-qdy30a-01"
#define LTE_MQTT_USER ""
#define LTE_MQTT_PASSWORD ""
#define LTE_MQTT_TOPIC "v1/devices/me/telemetry"
#define LTE_BAUD 115200
#define LTE_RETRY_MS 15000UL
#define SIMCOM_KEY PIN_D2
#define SIMCOM_SLEEP PIN_D1
/* Board-level polarity; set to 1 if RD2 drives an inverting transistor. */
#define SIMCOM_KEY_ACTIVE_HIGH 0
#define SIMCOM_SLEEP_AWAKE_HIGH 0
#define SIMCOM_KEY_PULSE_MS 100UL
#define SIMCOM_BOOT_MS 15000UL
#endif
