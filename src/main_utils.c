#include "main_utils.h"

int main_initialize_program(struct topic **topics_list, struct mosquitto **mosq_client)
{
	int id = 7;
	int rc;

	utils_get_data(topics_list);

	rc = mosquitto_lib_init();
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not initialize Mosquitto library. Return Code: %d\n", rc);
		return rc;
	}

	*mosq_client = mosquitto_new("test_client", true, &id);
	if (!(*mosq_client)) {
		log_event(LOG_EVENT_ERROR, "Could not create Mosquitto object. Return Code: %d\n", errno);
		mosquitto_lib_cleanup();
		return errno;
	}
	mosquitto_user_data_set(*mosq_client, *topics_list);
	mosquitto_connect_callback_set(*mosq_client, on_connect);
	mosquitto_message_callback_set(*mosq_client, on_message);

	rc = mosquitto_connect(*mosq_client, "localhost", 1883, 10);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not establish connection to Broker. Return Code: %d\n", rc);
		mosquitto_lib_cleanup();
		mosquitto_destroy(*mosq_client);
		return rc;
	}

	return rc;
}

int main_loop(struct mosquitto **mosq_client)
{
	int run = 1;
	int rc;

	rc = mosquitto_loop_start(*mosq_client);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not start mosquitto loop. Return Code: %d\n", rc);
		return rc;
	}
	getchar();
	// while(run) {
	//     sleep(5);
	// }

	rc = mosquitto_loop_stop(*mosq_client, true);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not stop mosquitto loop. Return Code: %d\n", rc);
		return rc;
	}

	return rc;
}

int main_deinitialize_program(struct topic **topic_list, struct mosquitto **mosq_client)
{
	int rc;

	rc = mosquitto_disconnect(*mosq_client);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not disconnect mosquitto. Return Code: %d\n", rc);
		return rc;
	}
	mosquitto_destroy(*mosq_client);
	mosquitto_lib_cleanup();

	topic_list_remove_all(topic_list);

	return MAIN_SUCCESS;
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	struct topic *tpc = (struct topic *)obj;
	while (tpc != NULL) {
		char *name = tpc->name;
		int qos	   = tpc->qos;
		if (mosquitto_subscribe(mosq, NULL, name, qos) != MOSQ_ERR_SUCCESS) {
			log_event(LOG_EVENT_WARNING, "Could not subscribe to topic: %s", name);
			continue;
		}
		log_event(LOG_EVENT_NOTICE, "Subscribed to: %s; QoS: %d", name, qos);
		print_events(tpc);
		tpc = tpc->next;
	}
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	char buffer[MAX_TOPIC_L];
	char *payload = (char *)msg->payload;
	char *topic   = msg->topic;
	snprintf(buffer, MAX_TOPIC_L, "MSG: Message received on topic '%s': %s\n", topic, payload);
	printf("%s\n", buffer);

	cJSON *payload_json = cJSON_Parse(payload);
	cJSON *data_json    = cJSON_GetObjectItemCaseSensitive(payload_json, "data");
	struct topic *tpc   = (struct topic *)obj;
	if (tpc = confirm_data(data_json, tpc, topic)) {
		cJSON *param_json = NULL;
		cJSON_ArrayForEach(param_json, data_json) send_event(param_json, tpc);
	}
	cJSON_Delete(payload_json);
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