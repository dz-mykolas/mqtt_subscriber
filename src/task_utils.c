#include "task_utils.h"

void log_event(int type, char *log)
{
	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("MQTT_subscriber", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
	char buffer[1024];
	snprintf(buffer, 100, "%s", log);
	syslog(type, "%s", buffer);
	closelog();
}

struct topic *create_node(char *topic_name, int qos)
{
    struct topic *t = (struct topic *)malloc(sizeof(struct topic));
    if (t == NULL)
        return NULL;
    strcpy(t->name, topic_name);
    t->qos = qos;
    t->next = NULL;
    t->events = NULL;
    t->max_events = 10;
    t->event_count = 0;
    return t;
}

void llist_add_end(struct topic **list, struct topic *t)
{
    struct topic *temp = *list;
    if (temp == NULL) {
        *list = t;
        return;
    }

    while (temp->next != NULL)
        temp = temp->next;
    temp->next = t;
}

void llist_remove_all(struct topic **list)
{
    struct topic *head = *list;
    while (head != NULL) {
        struct topic *temp;
        temp = head;
        head = head->next;
        if (temp->events != NULL)
            free(temp->events);
        free(temp);
    }
    *list = NULL;
}

int llist_add_event(struct topic *t, struct event e)
{
    if (t->event_count >= t->max_events)
        return -1;

    if (t->events == NULL) {
        t->events = (struct event *)calloc(t->max_events, sizeof(struct event));
        if (t->events == NULL)
            return -2;
    }

    t->events[t->event_count] = e;
    t->event_count++;
    return 0;
}

struct topic *find_topic(struct topic *topics_list, char *topic_name)
{
    struct topic *temp = topics_list;
    while (temp != NULL) {
        if (strcmp(temp->name, topic_name) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void send_mail()
{

}

bool compare_numeric(char *param)
{

}

bool compare_alphanumeric(char *param)
{

}

void send_event(char *param, struct topic *tpc)
{
    for (int i = 0; i < tpc->event_count; i++) {
        if (strcmp(tpc->events[i].param, param) != 0)
            continue;
        else if (tpc->events[i].type == type_numeric)
            if(compare_numeric(param)) {
                send_mail();
            }
        else if (tpc->events[i].type == type_alphanumeric)
            if (compare_alphanumeric(param)) {
                send_mail();
            }
    }
}