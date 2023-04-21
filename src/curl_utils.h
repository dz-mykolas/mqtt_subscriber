#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "task_utils.h"

#define MAX_SUBJECT_SIZE 512
#define MAX_PAYLOAD_SIZE 4096

int send_mail(struct event evt, struct email_node *eml, char *ref_value);
