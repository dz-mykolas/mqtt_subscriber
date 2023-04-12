#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <json-c>

#define LOG_EVENT_ERROR 3
#define LOG_EVENT_WARNING 4
#define LOG_EVENT_NOTICE 5
#define MAX_TOPIC_L 65536

struct topic {
    char name[MAX_TOPIC_L];
    int qos;
    struct topic *next;
};

struct event {
    char topic_name[MAX_TOPIC_L];
    int qos;
    struct topic *next;
};

void log_event(int type, char *log);
struct topic *create_node(char *topic_name, int qos);
void llist_add_end(struct topic **list, struct topic *t);
void llist_remove_all(struct topic **list);
