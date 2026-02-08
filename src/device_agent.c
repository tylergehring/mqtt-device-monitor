#include "../include/mqtt_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

static struct mosquitto *g_mosq = NULL;
static volatile bool running = true;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    running = false;
}

int main(int argc, char *argv[]) {
    char device_id[MAX_DEVICE_ID_LEN];
    int rc;
    
    // Generate device ID (use hostname or command line arg)
    if (argc > 1) {
        snprintf(device_id, sizeof(device_id), "%s", argv[1]);
    } else {
        snprintf(device_id, sizeof(device_id), "device_%d", getpid());
    }
    
    printf("Starting device agent: %s\n", device_id);
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize MQTT client
    rc = mqtt_init_client(&g_mosq, device_id);
    if (rc != 0) {
        return 1;
    }
    
    // Connect with LWT
    rc = mqtt_connect_with_lwt(g_mosq, device_id);
    if (rc != MOSQ_ERR_SUCCESS) {
        mqtt_cleanup(g_mosq);
        return 1;
    }
    
    // Main telemetry loop - publish metrics every 10 seconds
    int loop_count = 0;
    while (running) {
        // Process MQTT network traffic
        rc = mosquitto_loop(g_mosq, 1000, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "MQTT loop error: %s\n", mosquitto_strerror(rc));
            break;
        }
        
        // Publish telemetry every 10 seconds
        if (loop_count % 10 == 0) {
            device_metrics_t metrics;
            
            // Collect system metrics
            if (collect_system_metrics(&metrics, device_id) == 0) {
                // Publish telemetry
                mqtt_publish_telemetry(g_mosq, device_id, &metrics);
            } else {
                fprintf(stderr, "Warning: Failed to collect metrics\n");
            }
        }
        
        // Send heartbeat every 30 seconds
        if (loop_count % 30 == 0) {
            char topic[MAX_TOPIC_LEN];
            char payload[MAX_MESSAGE_LEN];
            time_t now = time(NULL);
            
            snprintf(topic, sizeof(topic), TOPIC_HEARTBEAT, device_id);
            snprintf(payload, sizeof(payload), 
                     "{\"heartbeat\": %d, \"timestamp\": %ld}", 
                     loop_count/30, now);
            
            mosquitto_publish(g_mosq, NULL, topic, strlen(payload), payload, 0, false);
            printf("Heartbeat %d sent\n", loop_count/30);
        }
        
        loop_count++;
        sleep(1);
    }
    
    // Graceful shutdown - publish offline status
    printf("Publishing offline status before exit...\n");
    mqtt_publish_status(g_mosq, device_id, DEVICE_STATE_OFFLINE);
    
    // Give time for message to be sent
    mosquitto_loop(g_mosq, 1000, 1);
    
    mqtt_cleanup(g_mosq);
    return 0;
}
