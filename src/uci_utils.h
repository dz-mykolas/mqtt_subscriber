#include <stdio.h>
#include <uci.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "task_utils.h"

typedef enum {
	EMAIL_NAME,
	EMAIL_SMTP_IP,
	EMAIL_SMTP_PORT,
	EMAIL_USERNAME,
	EMAIL_PASSWORD,
	EMAIL_SENDEREMAIL
} email_options;

typedef enum { TOPIC_NAME, TOPIC_QOS } topic_options;

typedef enum {
	EVENT_TOPIC_NAME,
	EVENT_COMPARISON_TYPE,
	EVENT_PARAMETER,
	EVENT_COMPARATOR,
	EVENT_REF_VALUE,
	EVENT_SENDER,
	EVENT_RECIPIENT
} event_options;

topic_options convert_topic_options(char *option);
event_options convert_event_options(char *option);

int utils_get_available_emails(struct email_node **list);
struct topic *create_topic_from_section(struct uci_section *section);
int create_event_from_section_helper(struct event *evt, struct topic **tpc, struct uci_option *o);
struct event create_event_from_section(struct topic **tpc, struct uci_section *section);

int utils_load_config(char *cfg, struct uci_context **ctx, struct uci_package **pkg);
int utils_get_topics(struct topic **topics_list);
int utils_get_events(struct topic **topics_list);
int utils_get_data(struct topic **topics_list);
