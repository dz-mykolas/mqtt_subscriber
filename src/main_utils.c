#include "main_utils.h"

/* ARGP */
const char *argp_program_version     = "mqtt_subscriber 1.0";
const char *argp_program_bug_address = "<none>";
static char doc[]		     = "MQTT Subscriber daemon that sends an email if event occurs";
static char args_doc[]		     = "product_id device_id device_secret";
static struct argp_option options[]  = { { 0 } };
static struct argp argp		     = { options, parse_opt, args_doc, "MQTT Subscriber" };

int main_init_mosquitto(struct mosquitto **mosq_client, int argc, char **argv)
{
	/* ARGUMENT PARSING */
	char *host     = argv[1];
	int port       = atoi(argv[2]);
	int tls	       = atoi(argv[3]);
	char *username = argv[4];
	char *password = argv[5];

	int id = 7;
	int rc = 1;
	rc     = mosquitto_lib_init();

	if (!port) {
		log_event(LOG_EVENT_ERROR, "MAIN/INIT: Could not parse port");
		rc = 1;
		return rc;
	}

	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Mosquitto: Could not initialize library. Return Code: %s",
			  mosquitto_strerror(rc));
		return rc;
	}

	*mosq_client = mosquitto_new("test_subscriber", true, &id);
	if (!(*mosq_client)) {
		log_event(LOG_EVENT_ERROR, "Mosquitto: Could not create Mosquitto object. Return Code: %d",
			  errno);
		mosquitto_lib_cleanup();
		return errno;
	}

	if (strcmp(username, "") != 0 && strcmp(password, "") != 0) {
		rc = mosquitto_username_pw_set(*mosq_client, username, password);
		if (rc != MOSQ_ERR_SUCCESS) {
			log_event(LOG_EVENT_ERROR, "Mosquitto: Unable to set username and/or password: %s",
				  mosquitto_strerror(rc));
			goto cleanup_client;
		}
	}

	if (tls) {
		rc = mosquitto_tls_set(*mosq_client, "/etc/certificates/ca.cert.pem", NULL,
				       "/etc/certificates/client.cert.pem",
				       "/etc/certificates/client.key.pem", NULL);
		if (rc != MOSQ_ERR_SUCCESS) {
			log_event(LOG_EVENT_ERROR, "Mosquitto: Unable to configure TLS: %s",
				  mosquitto_strerror(rc));
			goto cleanup_client;
		}
	}

	rc = mosquitto_connect(*mosq_client, host, port, 10);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Mosquitto: Could not establish connection to Broker: %s",
			  mosquitto_strerror(rc));
		goto cleanup_client;
	}

	return MAIN_SUCCESS;

cleanup_client:
	mosquitto_destroy(*mosq_client);
	mosquitto_lib_cleanup();
	return rc;
}

int main_init_sqlite(sqlite3 **db)
{
	char *errmsg;

	int rc = sqlite3_open("mqtt_subscriber_messages.db", db);
	if (rc != SQLITE_OK) {
		log_event(LOG_EVENT_ERROR, "Cannot open database: %s\n", sqlite3_errmsg(*db));
		sqlite3_close(*db);
		return 1;
	}

	const char *create_table_sql =
		"CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, time TEXT, topic TEXT, payload TEXT);";
	if (sqlite3_exec(*db, create_table_sql, 0, 0, &errmsg) != SQLITE_OK) {
		log_event(LOG_EVENT_ERROR, "SQL: %s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(*db);
		return 1;
	}

	return 0;
}

int main_initialize_program(struct data_pass_cb **data, struct mosquitto **mosq_client, int argc, char **argv)
{
	int rc			  = 1;
	struct sqlite3 *db	  = NULL;
	struct topic *topics_list = NULL;

	/* INIT ARGUMENTS */
	struct arguments arguments = { 0 };
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	/* SIGNAL HANDLING */
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sig_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* CURL INIT */
	CURLcode err_curl = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (err_curl != CURLE_OK) {
		log_event(LOG_EVENT_ERROR, curl_easy_strerror(err_curl));
		return err_curl;
	}

	/* SQLITE3 INIT */
	rc = main_init_sqlite(&db);
	if (rc)
		goto cleanup_curl;

	/* MOSQUITTO INIT */
	rc = main_init_mosquitto(mosq_client, argc, argv);
	if (rc)
		goto cleanup_sqlite;

	/* DATA SET */
	*data = (struct data_pass_cb *)malloc(sizeof(struct data_pass_cb));
	if (!data) {
		log_event(LOG_EVENT_ERROR, "MAIN/INIT: Failed to alocate memory for data");
		goto cleanup_mosquitto;
	}
	rc = utils_get_data(&topics_list);
	if (rc) {
		log_event(LOG_EVENT_ERROR, "MAIN/INIT: Failed to get data");
		goto cleanup_data;
	}
	(*data)->tpc = topics_list;
	(*data)->db  = db;
	mosquitto_user_data_set(*mosq_client, *data);
	mosquitto_connect_callback_set(*mosq_client, on_connect);
	mosquitto_message_callback_set(*mosq_client, on_message);

	return rc;

/* IF ANY FAILED */
cleanup_data:
	free(data);
cleanup_mosquitto:
	mosquitto_destroy(*mosq_client);
	mosquitto_lib_cleanup();
cleanup_sqlite:
	sqlite3_close(db);
cleanup_curl:
	curl_global_cleanup();

	return rc;
}

volatile sig_atomic_t running = 1;
int connected		      = 0;
int main_loop(struct mosquitto **mosq_client)
{
	int run = 1;
	int rc;

	rc = mosquitto_loop_start(*mosq_client);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not start mosquitto loop. Return Code: %d", rc);
		return rc;
	}
	time_t start_time   = time(NULL);
	int timeout_seconds = 5;
	while (running) {
		if (!connected && (time(NULL) - start_time) >= timeout_seconds) {
			log_event(LOG_EVENT_WARNING, "CONNACK not received within %d seconds. Disconnecting",
				  timeout_seconds);
			running = 0;
			break;
		}
		sleep(1);
	}

	rc = mosquitto_loop_stop(*mosq_client, true);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not stop mosquitto loop. Return Code: %d", rc);
		return rc;
	}

	return rc;
}

int main_deinitialize_program(struct data_pass_cb **data, struct mosquitto **mosq_client)
{
	int rc;

	curl_global_cleanup();

	rc = mosquitto_disconnect(*mosq_client);
	if (rc != MOSQ_ERR_SUCCESS) {
		log_event(LOG_EVENT_ERROR, "Could not disconnect mosquitto. Return Code: %d", rc);
		//return rc;
	}
	mosquitto_destroy(*mosq_client);
	mosquitto_lib_cleanup();
	sqlite3_close((*data)->db);
	topic_list_remove_all(&((*data)->tpc));
	free(*data);
	return MAIN_SUCCESS;
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	connected = 1;
	log_event(LOG_EVENT_NOTICE, "Mosquitto on_connect: %s", mosquitto_connack_string(rc));
	if (rc != 0) {
		running = 0;
		return;
	}

	struct data_pass_cb *data = (struct data_pass_cb *)obj;
	struct topic *tpc	  = data->tpc;
	log_event(LOG_EVENT_ERROR, "%s", tpc->name);
	while (tpc != NULL) {
		char *name = tpc->name;
		int qos	   = tpc->qos;
		if (mosquitto_subscribe(mosq, NULL, name, qos) != MOSQ_ERR_SUCCESS) {
			log_event(LOG_EVENT_WARNING, "Error subscribing: %s", mosquitto_strerror(rc));
			continue;
		}
		/*  DEBUGGING   */
		//log_event(LOG_EVENT_NOTICE, "Subscribed to: %s; QoS: %d", name, qos);
		//print_events(tpc);
		/*              */
		tpc = tpc->next;
	}
}

int add_msg_sqlite(char *topic, char *payload, sqlite3 *db)
{
	int rc;
	sqlite3_stmt *res;

	char *sql = "INSERT INTO messages (time, topic, payload) VALUES (?1, ?2, ?3);";
	rc	  = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	if (rc != SQLITE_OK) {
		log_event(LOG_EVENT_ERROR, "SQL: Failed to fetch data: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	int arr_size = 25;
	char current_time[arr_size];
	get_time_string(current_time, arr_size);

	sqlite3_bind_text(res, 1, current_time, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(res, 2, topic, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(res, 3, payload, -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(res);
	if (rc != SQLITE_DONE)
		log_event(LOG_EVENT_ERROR, "SQL: Execution failed: %s\n", sqlite3_errmsg(db));

	sqlite3_finalize(res);

	return 0;
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	struct data_pass_cb *data = (struct data_pass_cb *)obj;
	struct topic *tpc	  = data->tpc;
	struct sqlite3 *db	  = data->db;

	char buffer[MAX_TOPIC_L];
	char *payload = (char *)msg->payload;
	char *topic   = msg->topic;
	snprintf(buffer, MAX_TOPIC_L, "MSG: Message received on topic '%s': %s", topic, payload);
	printf("%s\n", buffer);
	if (add_msg_sqlite(topic, payload, db) != 0) {
		log_event(LOG_EVENT_WARNING, "MSG: Failed to add entry to database");
		return;
	}

	cJSON *payload_json = cJSON_Parse(payload);
	cJSON *data_json    = cJSON_GetObjectItemCaseSensitive(payload_json, "data");
	if (tpc = confirm_data(data_json, tpc, topic)) {
		cJSON *param_json = NULL;
		cJSON_ArrayForEach(param_json, data_json)
		{
			send_event(param_json, tpc);
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

void send_event(cJSON *param, struct topic *tpc)
{
	struct event curr;
	struct email_node *eml = NULL;
	utils_get_available_emails(&eml);
	FOR_EACH_EVENT(tpc->events, curr)
	{
		int mail_size = 50;
		char mail_msg[mail_size];
		char *event_string = curr.ref_string;
		char *json_string  = param->valuestring;
		double json_num	   = param->valuedouble;
		double event_num   = curr.ref_num;

		char *sender	 = curr.sender;
		char *comparator = curr.comp_sym;

		char *event_param = curr.param;
		char *json_param  = param->string;
		int json_type	  = param->type;
		int event_type	  = curr.type;

		struct email_node *needed = find_email(eml, sender);
		if (strcmp(event_param, json_param) != 0 || !needed)
			continue;

		int send = 0;
		if (event_type == TYPE_NUMERIC && json_type == cJSON_Number) {
			send = compare_numeric(comparator, json_num, event_num);
			snprintf(mail_msg, mail_size, "Ref: %.2f, Current: %.2f", event_num, json_num);
		} else if (event_type == TYPE_ALPHANUMERIC && json_type == cJSON_String) {
			send = compare_alphanumeric(comparator, json_string, event_string);
			snprintf(mail_msg, mail_size, "Ref: %s, Current: %s", event_string, json_string);
		}
		if (send)
			send_mail(curr, needed, mail_msg);
	}
	email_list_remove_all(&eml);
}

void sig_handler(int sig)
{
	switch (sig) {
	case SIGINT:
		log_event(LOG_EVENT_WARNING, "CTRL+C. Exiting");
		running = 0;
		break;
	case SIGTERM:
		log_event(LOG_EVENT_WARNING, "KILL Signal. Exiting");
		running = 0;
		break;
	}
}

error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	switch (key) {
	case ARGP_KEY_ARG:
		if (state->arg_num > 4) {
			printf("Too many arguments!\n");
			argp_usage(state);
			exit(EXIT_FAILURE);
		}
		arguments->args[state->arg_num] = arg;
		break;
	case ARGP_KEY_END:
		if (state->arg_num <= 4) {
			printf("Not enough arguments!\n");
			argp_usage(state);
			exit(EXIT_FAILURE);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
