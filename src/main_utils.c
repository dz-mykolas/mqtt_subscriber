#include "main_utils.h"

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    struct topic *temp = (struct topic *)obj;
    while (temp != NULL) {
        char buffer[MAX_TOPIC_L+10];
        char *name = temp->name;
        int qos = temp->qos;
        char *event;
        snprintf(buffer, MAX_TOPIC_L+10, "Subscribed to: %s; QoS: %d", name, qos);
        log_event(LOG_EVENT_NOTICE, buffer);
        if (temp->event_count > 0) {
            log_event(LOG_EVENT_NOTICE, "{");
            for (int i = 0; i < temp->event_count; i++) {
                event = temp->events[i].topic_name;
                snprintf(buffer, MAX_TOPIC_L+10, "    event: %s", event);
                log_event(LOG_EVENT_NOTICE, buffer);
            }
            log_event(LOG_EVENT_NOTICE, "}");
        }
        mosquitto_subscribe(mosq, NULL, name, qos);
        temp = temp->next;
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) 
{
    struct topic *tpc = (struct topic *)obj;

    char buffer[MAX_TOPIC_L];
    char *payload = (char *)msg->payload;
    snprintf(buffer, MAX_TOPIC_L, "Message received on topic '%s': %s\n", msg->topic, payload);
    printf("%s\n", buffer);

    cJSON *paylod_json = cJSON_Parse(payload);
    cJSON *data_json = cJSON_GetObjectItemCaseSensitive(paylod_json, "data");
    if (data_json) {
        tpc = find_topic(tpc, msg->topic);
        if (tpc) {
            cJSON *param_json = NULL;
            cJSON_ArrayForEach(param_json, data_json) {
                send_event(param_json->string, tpc);
            }
        }
    }
    cJSON_Delete(paylod_json);
}
