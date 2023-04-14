#include "task_utils.h"

void log_event(int type, const char *format, ...)
{
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("MQTT_subscriber", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

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

comparison_operator convert_comparator(char *param)
{
    if (strcmp(param, "<") == 0)
        return COMPARATOR_LESS;
    else if (strcmp(param, ">") == 0) {
        return COMPARATOR_MORE;
    } else if (strcmp(param, "==") == 0) {
        return COMPARATOR_EQUAL;
    } else if (strcmp(param, "!=") == 0) {
        return COMPARATOR_NOT_EQUAL;
    } else if (strcmp(param, "=<") == 0 || strcmp(param, "<=") == 0) {
        return COMPARATOR_LESS_EQUAL;
    } else if (strcmp(param, "=>") == 0 || strcmp(param, ">=") == 0) {
        return COMPARATOR_MORE_EQUAL;
    } else {
        log_event(1, "MSG: Param not found");
        return -1;
    }
}

bool compare_numeric(char *comparator, double value , double ref)
{
    switch (convert_comparator(comparator)) {
        case COMPARATOR_LESS: return value < ref;
        case COMPARATOR_MORE: return value > ref;
        case COMPARATOR_EQUAL: return value == ref;
        case COMPARATOR_NOT_EQUAL: return value != ref;
        case COMPARATOR_LESS_EQUAL: return value <= ref;
        case COMPARATOR_MORE_EQUAL: return value >= ref;
        default: return false;
    }
}

bool compare_alphanumeric(char *comparator, char *value, char *ref)
{
    switch (convert_comparator(comparator)) {
        //case COMPARATOR_LESS: return strcmp(value, ref) < 0;
        //case COMPARATOR_MORE: return strcmp(value, ref) > 0;
        case COMPARATOR_EQUAL: return strcmp(value, ref) == 0;
        case COMPARATOR_NOT_EQUAL: return strcmp(value, ref) != 0;
        //case COMPARATOR_LESS_EQUAL: return strcmp(value, ref) <= 0;
        //case COMPARATOR_MORE_EQUAL: return strcmp(value, ref) >= 0;
        default: return false;
    }
}

void send_mail()
{
    
}

void send_event(cJSON *param, struct topic *tpc)
{
    for (int i = 0; i < tpc->event_count; i++) {
        char *event_param = tpc->events[i].param;
        int event_type = tpc->events[i].type;
        double event_num = tpc->events[i].ref_num;
        char *event_string = tpc->events[i].ref_string;
        char *json_param = param->string;
        int json_type = param->type;

        char *comparator = tpc->events[i].comp_sym;
        if (strcmp(event_param, json_param) != 0)
            continue;
        else if (event_type == TYPE_NUMERIC && json_type == cJSON_Number) {
            if(compare_numeric(comparator, param->valuedouble, event_num))
                send_mail();
        }
        else if (event_type == TYPE_ALPHANUMERIC && json_type == cJSON_String) {
            if (compare_alphanumeric(comparator, param->valuestring, event_string))
                send_mail();
        }
    }
}
