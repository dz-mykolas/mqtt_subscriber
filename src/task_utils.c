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

struct email_node *create_email_node(char *configured_name, char *smtp_username, char *smtp_userpass,
				     char *smtp_ip, int smtp_port, char *sender)
{
	struct email_node *e = (struct email_node *)malloc(sizeof(struct email_node));
	if (e == NULL)
		return NULL;
	strcpy(e->configured_name, configured_name);
	strcpy(e->smtp_username, smtp_username);
	strcpy(e->smtp_userpass, smtp_userpass);
	strcpy(e->smtp_ip, smtp_ip);
	strcpy(e->sender, sender);
	e->smtp_port = smtp_port;
	e->next	     = NULL;
	return e;
}

void llist_add_email_end(struct email_node **list, struct email_node *e)
{
	struct email_node *temp = *list;
	if (temp == NULL) {
		*list = e;
		return;
	}

	while (temp->next != NULL)
		temp = temp->next;
	temp->next = e;
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

void email_list_remove_all(struct email_node **list)
{
	struct email_node *head     = *list;
	struct email_node *previous = NULL;
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

unsigned long hash_djb2(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

__uint64_t event_hash(struct event evt)
{
	__uint64_t hash = 0;

	char type[2];

	snprintf(type, 2, "%d", evt.type);
	char ref_string[100];
	if (evt.type == TYPE_NUMERIC) {
		snprintf(ref_string, sizeof(ref_string), "%f", evt.ref_num);
	} else
		snprintf(ref_string, sizeof(ref_string), "%s", evt.ref_string);
	int max_arr_size = 0;
	int i		 = MAX_RECIPIENTS;
	while (i > 0) {
		max_arr_size++;
		i /= 10;
	}
	char recipient_count[max_arr_size + 1];
	snprintf(recipient_count, sizeof(recipient_count), "%d", evt.recipient_count);
	hash += hash_djb2(evt.topic_name);
	hash += hash_djb2(type);
	hash += hash_djb2(evt.param);
	hash += hash_djb2(evt.comp_sym);
	hash += hash_djb2(ref_string);
	hash += hash_djb2(evt.sender);
	for (int i = 0; i < evt.recipient_count; i++) {
		hash += hash_djb2(evt.recipients[i]);
	}
	hash += hash_djb2(recipient_count);
	return hash;
}

int put_hashes(__uint64_t hashes[], int hashes_count, struct event_node *en)
{
	struct event evt;
	FOR_EACH_EVENT(en, evt)
	{
		hashes[hashes_count] = event_hash(evt);
		hashes_count++;
	}
	return hashes_count;
}

int find_duplicate(__uint64_t hashes[256], int hashes_count, struct event e)
{
	for (int i = 0; i < hashes_count; i++) {
		if (hashes[i] == event_hash(e))
			;
		return 1;
	}
	return 0;
}

int ullist_event_add_end(struct topic *t, struct event e)
{
	__uint64_t hashes[256];
	int hashes_count = 0;
	if (t->event_count >= MAX_EVENTS)
		return -1;

	struct event_node *n = t->events;
	if (t->event_count == 0) {
		n = create_events_node(e);
	} else {
		struct event_node *temp = n;
		while (temp->next != NULL) {
			hashes_count = put_hashes(hashes, hashes_count, temp);
			temp	     = temp->next;
		}

		if (find_duplicate(hashes, hashes_count, e) == 1) {
			log_event(LOG_EVENT_WARNING, "Duplicate event found");
			return 1;
		}

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

int compare_numeric(char *comparator, double value, double ref)
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
		return 0;
	}
}

int compare_alphanumeric(char *comparator, char *value, char *ref)
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
		return 0;
	}
}

struct email_node *find_email(struct email_node *eml, char *sender)
{
	FOR_EACH_NODE(eml)
	{
		if (strcmp(eml->configured_name, sender) == 0)
			return eml;
	}
	return NULL;
}

void get_time_string(char current_time[], int size)
{
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo   = localtime(&rawtime);
	int year   = timeinfo->tm_year + 1900;
	int month  = timeinfo->tm_mon + 1;
	int day	   = timeinfo->tm_mday;
	int hour   = timeinfo->tm_hour;
	int minute = timeinfo->tm_min;
	int second = timeinfo->tm_sec;

	snprintf(current_time, size, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}