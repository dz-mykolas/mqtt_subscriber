#include "main_utils.h"

int main() 
{
    log_event(LOG_EVENT_WARNING, "Program Starting!");

    int run = 1;
    int id = 7;
    int rc;
    struct topic *topics_list = NULL;
    utils_get_data(&topics_list);

    mosquitto_lib_init();
    struct mosquitto *mosq_client;
    mosq_client = mosquitto_new("test_client", true, &id);
    
    mosquitto_user_data_set(mosq_client, topics_list);
    mosquitto_connect_callback_set(mosq_client, on_connect);
    mosquitto_message_callback_set(mosq_client, on_message);

    rc = mosquitto_connect(mosq_client, "localhost", 1883, 10);
    if (rc) {
        char buffer[100];
        snprintf(buffer, 100, "Could not establish connection to Broker. Return Code: %d\n", rc);
        log_event(LOG_EVENT_ERROR, buffer);
        return -1;
    }

    /* KEEPALIVE */
    mosquitto_loop_start(mosq_client);

    getchar();
    // while(run) {
    //     sleep(5);
    // }

    mosquitto_loop_stop(mosq_client, true);

    /* DISCONNECT */
    mosquitto_disconnect(mosq_client);
    mosquitto_destroy(mosq_client);
    mosquitto_lib_cleanup();
    log_event(LOG_EVENT_WARNING, "Program Finished!");
    llist_remove_all(&topics_list);
    return 0;
}
