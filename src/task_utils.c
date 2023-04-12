#include "task_utils.h"

void log_event(int type, char *log)
{
	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("tuya_daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
	char buffer[100];
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
        free(temp);
    }
    *list = NULL;
}