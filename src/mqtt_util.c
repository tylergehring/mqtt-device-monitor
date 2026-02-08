#include "../include/mqtt_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int mqtt_init_client(struct mosquitto **mosq, const char *client_id) {
    mosquitto_lib_init();

    *mosq = mosquitto_new(client_id, true, NULL);
    if (!*mosq) {
        fprintf(stderr, "Error: Failed to create mosquitto instance\n");
        return -1;
    }
    
    printf("MQTT client initialized with ID: %s\n", client_id);
    return 0;
}

int mqtt_connect_with_lwt(struct mosquitto *mosq, const char *device_id) {
    char lwt_topic[MAX_TOPIC_LEN];
    char lwt_payload[] = "{\"state\":\"offline\",\"timestamp\":%ld}";
    char lwt_message[MAX_MESSAGE_LEN];
    time_t now = time(NULL);

    //format topic and payload for LWT (???)
    snprintf(lwt_topic, sizeof(lwt_topic), TOPIC_STATUS, device_id);
    snprintf(lwt_message, sizeof(lwt_message), lwt_payload, now);
    
    // Set Last Will and Testament (???)
    int rc = mosquitto_will_set(mosq, lwt_topic, strlen(lwt_message), 
                                lwt_message, 1, true);

    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error setting LWT: %s\n", mosquitto_strerror(rc));
        return rc;
    }
    
    //contect to broker
    rc = mosquitto_connect(mosq, MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_KEEPALIVE);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error connecting to broker: %s\n", mosquitto_strerror(rc));
        return rc;
    }

     printf("Connected to MQTT broker with LWT configured\n");
    
    // Publish online status immediately after connecting
    rc = mqtt_publish_status(mosq, device_id, DEVICE_STATE_ONLINE);
    return rc;
}

int mqtt_publish_status(struct mosquitto *mosq, const char *device_id, device_state_t state){
    char topic[MAX_TOPIC_LEN];
    char payload[MAX_MESSAGE_LEN];
    const char *state_str;
    time_t now = time(NULL);

    //convert state to string
    switch (state) {
        case DEVICE_STATE_ONLINE: state_str = "online"; break;
        case DEVICE_STATE_OFFLINE: state_str = "offline"; break;
        case DEVICE_STATE_ERROR: state_str = "error"; break;
        default: state_str = "unknown";
    }

    snprintf(topic, sizeof(topic), TOPIC_STATUS, device_id);
    snprintf(payload, sizeof(payload), 
             "{\"state\": \"%s\", \"timestamp\": %ld}", 
             state_str, now);

    int rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 1, true);
    if (rc == MOSQ_ERR_SUCCESS) {
        printf("Status published: %s -> %s\n", device_id, state_str);
    }
    return rc;
}

// Create JSON payload from metrics structure
char* mqtt_create_json_payload(const device_metrics_t *metrics) {
    if (!metrics) {
        return NULL;
    }
    
    char *json = malloc(MAX_MESSAGE_LEN);
    if (!json) {
        fprintf(stderr, "Error: Memory allocation failed for JSON payload\n");
        return NULL;
    }
    
    snprintf(json, MAX_MESSAGE_LEN,
        "{"
        "\"timestamp\":%lu,"
        "\"device_id\":\"%s\","
        "\"uptime\":%u,"
        "\"cpu_temp\":%.2f,"
        "\"memory\":{"
            "\"total\":%lu,"
            "\"used\":%lu,"
            "\"free\":%lu"
        "},"
        "\"network\":{"
            "\"rx_bytes\":%lu,"
            "\"tx_bytes\":%lu"
        "}"
        "}",
        metrics->timestamp,
        metrics->device_id,
        metrics->uptime_seconds,
        metrics->cpu_temp_celsius,
        metrics->memory.total_bytes,
        metrics->memory.used_bytes,
        metrics->memory.free_bytes,
        metrics->network.rx_bytes,
        metrics->network.tx_bytes
    );
    
    return json;
}

// Publish telemetry data to MQTT broker
int mqtt_publish_telemetry(struct mosquitto *mosq, const char *device_id, const device_metrics_t *metrics) {
    char topic[MAX_TOPIC_LEN];
    char *payload;
    int rc;
    
    if (!mosq || !device_id || !metrics) {
        return -1;
    }
    
    //Create topic
    snprintf(topic, sizeof(topic), TOPIC_TELEMETRY, device_id);
    
    //Create JSON payload
    payload = mqtt_create_json_payload(metrics);
    if (!payload) {
        return -1;
    }
    
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc == MOSQ_ERR_SUCCESS) {
        printf("Telemetry published: %s\n", device_id);
    } else {
        fprintf(stderr, "Error publishing telemetry: %s\n", mosquitto_strerror(rc));
    }
    
    free(payload);
    return rc;
}

void mqtt_cleanup(struct mosquitto *mosq) {
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
    printf("MQTT cleanup complete\n");
}