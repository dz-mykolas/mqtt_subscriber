#include "main_utils.h"

int main_initialize_program(struct topic **topics_list, struct mosquitto **mosq_client)
{
	int id = 7;
	int rc;

	CURLcode err_curl = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (err_curl != CURLE_OK) {
		log_event(LOG_EVENT_ERROR, curl_easy_strerror(err_curl));
		return err_curl;
	}

	rc = mosquitto_lib_init();
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not initialize Mosquitto library. Return Code: %d\n", rc);
		curl_global_cleanup();
		return rc;
	}

	*mosq_client = mosquitto_new("test_client", true, &id);
	if (!(*mosq_client)) {
		log_event(LOG_EVENT_ERROR, "Could not create Mosquitto object. Return Code: %d\n", errno);
		curl_global_cleanup();
		mosquitto_lib_cleanup();
		return errno;
	}

	rc = mosquitto_connect(*mosq_client, "localhost", 1883, 10);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not establish connection to Broker. Return Code: %d\n", rc);
		curl_global_cleanup();
		mosquitto_lib_cleanup();
		mosquitto_destroy(*mosq_client);
		return rc;
	}

	utils_get_data(topics_list);
	mosquitto_user_data_set(*mosq_client, *topics_list);
	mosquitto_connect_callback_set(*mosq_client, on_connect);
	mosquitto_message_callback_set(*mosq_client, on_message);

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

	curl_global_cleanup();

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

	cJSON *payload_json    = cJSON_Parse(payload);
	cJSON *data_json       = cJSON_GetObjectItemCaseSensitive(payload_json, "data");
	struct topic *tpc      = (struct topic *)obj;
	struct email_node *eml = NULL;
	utils_get_available_emails(&eml);
	if (tpc = confirm_data(data_json, tpc, topic)) {
		cJSON *param_json = NULL;
		cJSON_ArrayForEach(param_json, data_json)
		{
			send_event(param_json, tpc, eml);
		}
	}
	cJSON_Delete(payload_json);
}

struct topic *confirm_data(cJSON *data, struct topic *tpc, char *topic)
{
	if (data) {
		tpc = find_topic(tpc, topic);
		if (tpc)
			return tpc;
		log_event(LOG_EVENT_WARNING, "MSG: Topic not found");
	} else {
		log_event(LOG_EVENT_WARNING, "MSG: Data invalid");
	}
	return NULL;
}

void send_event(cJSON *param, struct topic *tpc, struct email_node *eml)
{
	struct event curr;
	FOR_EACH_EVENT(tpc->events, curr)
	{
		char *sender		  = curr.sender;
		char *event_param	  = curr.param;
		int event_type		  = curr.type;
		double event_num	  = curr.ref_num;
		char *event_string	  = curr.ref_string;
		char *comparator	  = curr.comp_sym;
		char *json_param	  = param->string;
		int json_type		  = param->type;
		struct email_node *needed = find_email(eml, sender);

		if (strcmp(event_param, json_param) != 0)
			continue;
		if (!needed) {
            continue;
        }
		if (event_type == TYPE_NUMERIC && json_type == cJSON_Number) {
			if (compare_numeric(comparator, param->valuedouble, event_num))
				send_mail(curr, needed, "TestN");
		} else if (event_type == TYPE_ALPHANUMERIC && json_type == cJSON_String) {
			if (compare_alphanumeric(comparator, param->valuestring, event_string))
				send_mail(curr, needed, "TestA");
		}
	}
}
