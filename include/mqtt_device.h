#ifndef MQTT_DEVICE_H
#define MQTT_DEVICE_H

#include <mosquitto.h>
#include <stdbool.h>
#include <stdint.h>

/* Configuration Constants */
#define MAX_DEVICE_ID_LEN 64
#define MAX_TOPIC_LEN 256
#define MAX_MESSAGE_LEN 1024
#define MQTT_BROKER_HOST "localhost"
#define MQTT_BROKER_PORT 1883
#define MQTT_KEEPALIVE 60

/* Topic Templates */
#define TOPIC_TELEMETRY "devices/%s/telemetry"
#define TOPIC_STATUS    "devices/%s/status"  
#define TOPIC_HEARTBEAT "devices/%s/heartbeat"

/* Device States */
typedef enum {
    DEVICE_STATE_OFFLINE = 0,
    DEVICE_STATE_ONLINE = 1,
    DEVICE_STATE_ERROR = 2
} device_state_t;

/* System Metrics Structure */
typedef struct {
    uint64_t timestamp;
    char device_id[MAX_DEVICE_ID_LEN];
    uint32_t uptime_seconds;
    float cpu_temp_celsius;
    struct {
        uint64_t total_bytes;
        uint64_t used_bytes; 
        uint64_t free_bytes;
    } memory;
    struct {
        uint64_t rx_bytes;
        uint64_t tx_bytes;
        uint32_t rx_packets;
        uint32_t tx_packets;
    } network;
} device_metrics_t;

/* Function Declarations */
/* mqtt_utils.c */
int mqtt_init_client(struct mosquitto **mosq, const char *client_id);
int mqtt_connect_with_lwt(struct mosquitto *mosq, const char *device_id);
int mqtt_publish_status(struct mosquitto *mosq, const char *device_id, device_state_t state);
int mqtt_publish_telemetry(struct mosquitto *mosq, const char *device_id, const device_metrics_t *metrics);
char* mqtt_create_json_payload(const device_metrics_t *metrics);
void mqtt_cleanup(struct mosquitto *mosq);

/* metrics.c */
int collect_system_metrics(device_metrics_t *metrics, const char *device_id);
int read_cpu_temperature(float *temp);
int read_memory_stats(uint64_t *total, uint64_t *used, uint64_t *free);
int read_network_stats(uint64_t *rx_bytes, uint64_t *tx_bytes);
uint32_t get_uptime_seconds(void);
uint64_t get_timestamp_unix(void);

#endif /* MQTT_DEVICE_H */