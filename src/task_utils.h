#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdarg.h>

#define LOG_EVENT_ERROR	  3
#define LOG_EVENT_WARNING 4
#define LOG_EVENT_NOTICE  5
#define MAX_TOPIC_L	  1024 /* 65536 MQTT max allowed */
#define MAX_EVENTS 16
#define MAX_EVENTS_NODE 4
#define MAX_RECIPIENTS	  10

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
	char recipient[100][MAX_RECIPIENTS];
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
	struct topic *next;
	struct event_node *events;
    int event_count;
};

#define FOR_EACH_NODE(_node) \
    for (; _node != NULL; _node = _node->next)

#define FOR_EACH_EVENT(_node, _event) \
    for (struct event_node *_temp = (_node); _temp != NULL; _temp = _temp->next) \
        for (int _i = 0; _i < _temp->count && (_event = _temp->events[_i], 1); ++_i)

void log_event(int type, const char *format, ...);
struct topic *find_topic(struct topic *topics_list, char *topic_name);
comparison_operator convert_comparator(char *param);
bool compare_numeric(char *comparator, double value, double ref);
void send_mail();
void send_event(cJSON *param, struct topic *tpc);
void print_events(struct topic *tpc);

struct event_node *create_events_node();
struct topic *create_node(char *topic_name, int qos);
void llist_add_end(struct topic **list, struct topic *t);
int ullist_event_add_end(struct topic *t, struct event e);
void topic_list_remove_all(struct topic **list);
void event_list_remove_all(struct event_node **list);
