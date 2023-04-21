#include <mosquitto.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <curl/curl.h>

#include "uci_utils.h"
#include "curl_utils.h"
#include "task_utils.h"
#include "cJSON.h"

#define MAIN_SUCCESS	    0
#define MAIN_FAILURE_INIT   1
#define MAIN_FAILURE_LOOP   2
#define MAIN_FAILURE_DEINIT 3

void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
struct topic *confirm_data(cJSON *data, struct topic *tpc, char *topic);
void send_event(cJSON *param, struct topic *tpc, struct email_node *eml);

int main_initialize_program(struct topic **topics_list, struct mosquitto **mosq_client);
int main_loop(struct mosquitto **mosq_client);
int main_deinitialize_program(struct topic **topics_list, struct mosquitto **mosq_client);
