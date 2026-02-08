#include "../include/mqtt_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Get current Unix timestamp
uint64_t get_timestamp_unix(void) {
    return (uint64_t)time(NULL);
}

// Get system uptime in seconds
uint32_t get_uptime_seconds(void) {
    FILE *fp;
    double uptime;
    
    fp = fopen("/proc/uptime", "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open /proc/uptime\n");
        return 0;
    }
    
    if (fscanf(fp, "%lf", &uptime) != 1) {
        fprintf(stderr, "Error: Cannot read uptime\n");
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    return (uint32_t)uptime;
}

// Read CPU temperature (Celsius)
int read_cpu_temperature(float *temp) {
    FILE *fp;
    int temp_millidegrees;
    
    // Try thermal_zone0 (most common location)
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) {
        // Fallback: try hwmon sensors (alternative location)
        fp = fopen("/sys/class/hwmon/hwmon0/temp1_input", "r");
        if (!fp) {
            fprintf(stderr, "Warning: Cannot read CPU temperature\n");
            *temp = 0.0;
            return -1;
        }
    }
    
    if (fscanf(fp, "%d", &temp_millidegrees) != 1) {
        fprintf(stderr, "Warning: Cannot parse temperature\n");
        fclose(fp);
        *temp = 0.0;
        return -1;
    }
    
    fclose(fp);
    *temp = temp_millidegrees / 1000.0;  // Convert millidegrees to degrees
    return 0;
}

// Read memory statistics from /proc/meminfo
int read_memory_stats(uint64_t *total, uint64_t *used, uint64_t *free) {
    FILE *fp;
    char line[256];
    uint64_t mem_total = 0, mem_free = 0, mem_available = 0, buffers = 0, cached = 0;
    
    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open /proc/meminfo\n");
        return -1;
    }
    
    // Parse /proc/meminfo line by line
    while (fgets(line, sizeof(line), fp)) {
        // Look for key fields (values are in kB)
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemFree: %lu kB", &mem_free) == 1) continue;
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) continue;
        if (sscanf(line, "Buffers: %lu kB", &buffers) == 1) continue;
        if (sscanf(line, "Cached: %lu kB", &cached) == 1) continue;
    }
    fclose(fp);
   
    *total = mem_total * 1024; // Convert kB to bytes
    if (mem_available > 0) {
        *free = mem_available * 1024;
    } else {
        *free = (mem_free + buffers + cached) * 1024;
    }
    *used = *total - *free;
    
    return 0;
}

// Read network statistics from /proc/net/dev
int read_network_stats(uint64_t *rx_bytes, uint64_t *tx_bytes) {
    FILE *fp;
    char line[512];
    char interface[32];
    uint64_t rx, tx;
    uint64_t total_rx = 0, total_tx = 0;
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open /proc/net/dev\n");
        return -1;
    }
    
    fgets(line, sizeof(line), fp); //skip line
    fgets(line, sizeof(line), fp); //skip line
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%31[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu",
                   interface, &rx, &tx) == 3) {
            if (strncmp(interface, "lo", 2) == 0) {
                continue;
            }
            
            total_rx += rx;
            total_tx += tx;
        }
    }
    fclose(fp);
    
    *rx_bytes = total_rx;
    *tx_bytes = total_tx;
    return 0;
}

// Collect all system metrics into the structure
int collect_system_metrics(device_metrics_t *metrics, const char *device_id) {
    if (!metrics || !device_id) {
        return -1;
    }
    
    // Fill in device ID
    strncpy(metrics->device_id, device_id, MAX_DEVICE_ID_LEN - 1);
    metrics->device_id[MAX_DEVICE_ID_LEN - 1] = '\0';
    metrics->timestamp = get_timestamp_unix();
    metrics->uptime_seconds = get_uptime_seconds();
    read_cpu_temperature(&metrics->cpu_temp_celsius);

    if (read_memory_stats(&metrics->memory.total_bytes,
                          &metrics->memory.used_bytes,
                          &metrics->memory.free_bytes) != 0) {
        fprintf(stderr, "Warning: Failed to read memory stats\n");
    }
    
    if (read_network_stats(&metrics->network.rx_bytes,
                           &metrics->network.tx_bytes) != 0) {
        fprintf(stderr, "Warning: Failed to read network stats\n");
    }
    
    //don't track packets in /proc/net/dev simple parsing, set to 0
    metrics->network.rx_packets = 0;
    metrics->network.tx_packets = 0;
    return 0;
}
