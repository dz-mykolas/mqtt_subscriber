#include <mosquitto.h>
#include <unistd.h>
#include <stdio.h>
#include <cjson/cJSON.h>

#include "uci_utils.h"

void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);