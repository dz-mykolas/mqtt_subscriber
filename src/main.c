#include "main_utils.h"

int main(int argc, char **argv)
{
	log_event(LOG_EVENT_WARNING, "Program Starting!");
	struct topic *topic_list = NULL;
	struct mosquitto *mosq_client;

	if (main_initialize_program(&topic_list, &mosq_client, argc, argv) != MAIN_SUCCESS)
		return MAIN_FAILURE_INIT;

	if (main_loop(&mosq_client) != MAIN_SUCCESS)
		return MAIN_FAILURE_LOOP;

	if (main_deinitialize_program(&topic_list, &mosq_client) != MAIN_SUCCESS)
		return MAIN_FAILURE_DEINIT;

	log_event(LOG_EVENT_WARNING, "Program Finished!");
	return MAIN_SUCCESS;
}
