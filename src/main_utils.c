#include "main_utils.h"

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    struct topic *temp = (struct topic *)obj;
    while (temp != NULL) {
        char buffer[MAX_TOPIC_L+10];
        char *name = temp->name;
        int qos = temp->qos;
        
        snprintf(buffer, MAX_TOPIC_L+10, "Subscribed to: %s; QoS: %d", name, qos);
        log_event(LOG_EVENT_NOTICE, buffer);
        if (temp->event_count > 0) {
            log_event(LOG_EVENT_NOTICE, "    Events: ");
            for (int i = 0; i < temp->event_count; i++) {
                char *event_param = temp->events[i].param;
                char *event_sym = temp->events[i].comp_sym;
                snprintf(buffer, MAX_TOPIC_L+10, "        event: %s %s", event_param, event_sym);
                log_event(LOG_EVENT_NOTICE, buffer);
            }
        } else {
            log_event(LOG_EVENT_NOTICE, "    No events");
        }
        mosquitto_subscribe(mosq, NULL, name, qos);
        temp = temp->next;
    }
}

struct topic *confirm_data(cJSON *data, struct topic *tpc, char *topic)
{
    if (data) {
        tpc = find_topic(tpc, topic);
        if (tpc)
            return tpc;
        log_event(LOG_WARNING, "MSG: Topic not found");
    } else {
        log_event(LOG_WARNING, "MSG: Data invalid");
    }
    return NULL;
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) 
{
    char buffer[MAX_TOPIC_L];
    char *payload = (char *)msg->payload;
    char *topic = msg->topic;
    snprintf(buffer, MAX_TOPIC_L, "MSG: Message received on topic '%s': %s\n", topic, payload);
    printf("%s\n", buffer);

    cJSON *payload_json = cJSON_Parse(payload);
    cJSON *data_json = cJSON_GetObjectItemCaseSensitive(payload_json, "data");
    struct topic *tpc = (struct topic *)obj;
    if (tpc = confirm_data(data_json, tpc, topic)) {
        cJSON *param_json = NULL;
        cJSON_ArrayForEach(param_json, data_json)
            send_event(param_json, tpc);
    }
    cJSON_Delete(payload_json);
}
