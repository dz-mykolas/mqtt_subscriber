#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define LOG_EVENT_ERROR 3
#define LOG_EVENT_WARNING 4
#define LOG_EVENT_NOTICE 5
#define MAX_TOPIC_L 1024 //65536
#define MAX_RECIPIENTS 10

enum {
    type_numeric,
    type_alphanumeric
};

struct event {
    char topic_name[MAX_TOPIC_L];
    int type;
    char param[50];
    char comp_sym[3];
    int ref;
    char sender[100];
    char recipient[100][MAX_RECIPIENTS]; 
    int recipient_count;
};

struct topic {
    char name[MAX_TOPIC_L];
    int qos;
    struct topic *next;

    struct event *events;
    int max_events;
    int event_count;
};

void log_event(int type, char *log);
struct topic *create_node(char *topic_name, int qos);
void llist_add_end(struct topic **list, struct topic *t);
void llist_remove_all(struct topic **list);
int llist_add_event(struct topic *t, struct event e);
struct topic *find_topic(struct topic *topics_list, char *topic_name);
void send_event(char *param, struct topic *tpc);