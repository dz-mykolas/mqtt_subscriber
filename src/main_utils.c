#include "main_utils.h"

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    struct topic *topics_list = NULL;
    utils_get_topics(&topics_list);
    struct topic *temp = topics_list;
    while (temp != NULL) {
        char buffer[MAX_TOPIC_L+10];
        char *name = temp->name;
        int qos = temp->qos;
        snprintf(buffer, MAX_TOPIC_L+10, "%s; %d", name, qos);
        log_event(LOG_EVENT_ERROR, buffer);
        mosquitto_subscribe(mosq, NULL, name, qos);
        temp = temp->next;
    }
    llist_remove_all(&topics_list);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    char buffer[MAX_TOPIC_L+10];
    snprintf(buffer, MAX_TOPIC_L+10, "Message received on topic '%s': %s\n", msg->topic, (char *)msg->payload);
    log_event(LOG_EVENT_NOTICE, buffer);
}
