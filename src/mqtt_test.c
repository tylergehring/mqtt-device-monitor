#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

int main(){
    struct mosquitto *mosq = NULL;
    int rc;

    mosquitto_lib_init();
    mosq = mosquitto_new("test-client", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error: Failed to create mosquitto instance.\n");
        return 1;
    }

    //broker connection
    rc = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Failed to connect to broker: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    printf("Connected To MQTT Broker\n");

    //test message
    rc = mosquitto_publish(mosq, NULL, "test/c-client",
                            strlen("Hello world!"), "Hello world!",
                            0, false);
    if (rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error: Failed to publish message: %s\n", mosquitto_strerror(rc));
    } else {
        printf("Message published successfully!\n");
    }

    //clean up
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("MQTT test complete.\n");
    return 0;
}