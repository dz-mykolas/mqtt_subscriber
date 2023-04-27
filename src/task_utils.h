#ifndef TASK_UTILS_H
#define TASK_UTILS_H

#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

#include "cJSON.h"

#define LOG_EVENT_ERROR	  3
#define LOG_EVENT_WARNING 4
#define LOG_EVENT_NOTICE  5

#define MAX_TOPIC_L	1024 /* 65536 MQTT max allowed */
#define MAX_EVENTS	16
#define MAX_EVENTS_NODE 4
#define MAX_RECIPIENTS	10
#define MAX_TABLE_SIZE	128

enum { TYPE_NUMERIC, TYPE_ALPHANUMERIC };

typedef enum {
	COMPARATOR_LESS,
	COMPARATOR_MORE,
	COMPARATOR_EQUAL,
	COMPARATOR_NOT_EQUAL,
	COMPARATOR_LESS_EQUAL,
	COMPARATOR_MORE_EQUAL
} comparison_operator;

struct event {
	char topic_name[MAX_TOPIC_L];
	int type;
	char param[50];
	char comp_sym[3];
	double ref_num;
	char ref_string[100];
	char sender[100];
	char recipients[MAX_RECIPIENTS][100];
	int recipient_count;
};

struct event_node {
	int count;
	struct event events[MAX_EVENTS_NODE];
	struct event_node *next;
};

struct topic {
	char name[MAX_TOPIC_L];
	int qos;
	struct event_node *events;
	int event_count;
	struct topic *next;
};

struct email_node {
	char configured_name[32];
	char smtp_username[32];
	char smtp_userpass[64];
	char smtp_ip[64];
	int smtp_port;
	char sender[64];
	struct email_node *next;
};

struct event_hash_entry {
    char *key;
    struct event_node *value;
};

#define FOR_EACH_NODE(_node) for (; _node != NULL; _node = _node->next)

#define FOR_EACH_EVENT(_node, _event)                                                                        \
	for (struct event_node *_temp = (_node); _temp != NULL; _temp = _temp->next)                         \
		for (int _i = 0; _i < _temp->count && (_event = _temp->events[_i], 1); ++_i)

void log_event(int type, const char *format, ...);
struct topic *find_topic(struct topic *topics_list, char *topic_name);
comparison_operator convert_comparator(char *param);
int compare_numeric(char *comparator, double value, double ref);
int compare_alphanumeric(char *comparator, char *value, char *ref);
void print_events(struct topic *tpc);
struct email_node *find_email(struct email_node *eml, char *sender);
void get_time_string(char current_time[], int size);

struct email_node *create_email_node(char *configured_name, char *smtp_username, char *smtp_userpass,
				     char *smtp_ip, int smtp_port, char *sender);
void llist_add_email_end(struct email_node **list, struct email_node *e);
struct event_node *create_events_node();
struct topic *create_node(char *topic_name, int qos);
void llist_add_end(struct topic **list, struct topic *t);
int ullist_event_add_end(struct topic *t, struct event e);
void topic_list_remove_all(struct topic **list);
void event_list_remove_all(struct event_node **list);
void email_list_remove_all(struct email_node **list);

#endif