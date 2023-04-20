#include "task_utils.h"

void print_events(struct topic *tpc)
{
	if (tpc->event_count > 0) {
		log_event(LOG_EVENT_NOTICE, "%10s", "Events");
		struct event evt;
		FOR_EACH_EVENT(tpc->events, evt)
		{
			char *event_param = evt.param;
			char *event_sym	  = evt.comp_sym;
			log_event(LOG_EVENT_NOTICE, "%14s %s %s", "event:", event_param, event_sym);
		}
	} else {
		log_event(LOG_EVENT_NOTICE, "%13s", "No events");
	}
}

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

struct event_node *create_events_node(struct event evt)
{
	struct event_node *n = (struct event_node *)malloc(sizeof(struct event_node));
	if (n == NULL)
		return NULL;
	n->count     = 1;
	n->events[0] = evt;
	n->next	     = NULL;
	return n;
}

struct topic *create_node(char *topic_name, int qos)
{
	struct topic *t = (struct topic *)malloc(sizeof(struct topic));
	if (t == NULL)
		return NULL;
	strcpy(t->name, topic_name);
	t->qos	       = qos;
	t->next	       = NULL;
	t->events      = NULL;
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

void event_list_remove_all(struct event_node **list)
{
	struct event_node *head	    = *list;
	struct event_node *previous = NULL;
	FOR_EACH_NODE(head)
	{
		if (previous)
			free(previous);
		previous = head;
	}
	if (previous)
		free(previous);
	*list = NULL;
}

void topic_list_remove_all(struct topic **list)
{
	struct topic *head     = *list;
	struct topic *previous = NULL;
	FOR_EACH_NODE(head)
	{
		if (head->events != NULL)
			event_list_remove_all(&(head->events));
		if (previous)
			free(previous);
		previous = head;
	}
	if (previous)
		free(previous);
	*list = NULL;
}

int ullist_event_add_end(struct topic *t, struct event e)
{
	if (t->event_count >= MAX_EVENTS)
		return -1;

	struct event_node *n = t->events;
	if (t->event_count == 0) {
		n = create_events_node(e);
	} else {
		struct event_node *temp = n;
		while (temp->next != NULL)
			temp = temp->next;

		if (temp->count < MAX_EVENTS_NODE) {
			temp->events[temp->count] = e;
			temp->count++;
		} else {
			temp->next = create_events_node(e);
		}
	}

	t->events = n;
	t->event_count++;
	return 0;
}

struct topic *find_topic(struct topic *topics_list, char *topic_name)
{
	struct topic *temp = topics_list;
	FOR_EACH_NODE(temp)
	{
		if (strcmp(temp->name, topic_name) == 0)
			return temp;
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
		return -1;
	}
}

bool compare_numeric(char *comparator, double value, double ref)
{
	switch (convert_comparator(comparator)) {
	case COMPARATOR_LESS:
		return value < ref;
	case COMPARATOR_MORE:
		return value > ref;
	case COMPARATOR_EQUAL:
		return value == ref;
	case COMPARATOR_NOT_EQUAL:
		return value != ref;
	case COMPARATOR_LESS_EQUAL:
		return value <= ref;
	case COMPARATOR_MORE_EQUAL:
		return value >= ref;
	default:
		return false;
	}
}

bool compare_alphanumeric(char *comparator, char *value, char *ref)
{
	switch (convert_comparator(comparator)) {
	//case COMPARATOR_LESS: return strcmp(value, ref) < 0;
	//case COMPARATOR_MORE: return strcmp(value, ref) > 0;
	case COMPARATOR_EQUAL:
		return strcmp(value, ref) == 0;
	case COMPARATOR_NOT_EQUAL:
		return strcmp(value, ref) != 0;
	//case COMPARATOR_LESS_EQUAL: return strcmp(value, ref) <= 0;
	//case COMPARATOR_MORE_EQUAL: return strcmp(value, ref) >= 0;
	default:
		return false;
	}
}

void send_mail()
{
    
}

void send_event(cJSON *param, struct topic *tpc)
{
	struct event curr;
	FOR_EACH_EVENT(tpc->events, curr)
	{
		char *event_param  = curr.param;
		int event_type	   = curr.type;
		double event_num   = curr.ref_num;
		char *event_string = curr.ref_string;
		char *comparator   = curr.comp_sym;
		char *json_param   = param->string;
		int json_type	   = param->type;

		if (strcmp(event_param, json_param) != 0) {
			continue;
		} else if (event_type == TYPE_NUMERIC && json_type == cJSON_Number) {
			if (compare_numeric(comparator, param->valuedouble, event_num))
				send_mail();
		} else if (event_type == TYPE_ALPHANUMERIC && json_type == cJSON_String) {
			if (compare_alphanumeric(comparator, param->valuestring, event_string))
				send_mail();
		}
	}
}
