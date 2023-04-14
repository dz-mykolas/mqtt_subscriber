#include <stdio.h>
#include <uci.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "task_utils.h"

typedef enum {
    TOPIC_NAME
} topic_options;

typedef enum {
    EVENT_TOPIC_NAME,
    EVENT_COMPARISON_TYPE,
    EVENT_PARAMETER,
    EVENT_COMPARATOR,
    EVENT_REF_VALUE,
    EVENT_SENDER,
    EVENT_RECIPIENT
} event_options;


int utils_get_topics(struct topic **topics_list);
int utils_get_data(struct topic **topics_list);
