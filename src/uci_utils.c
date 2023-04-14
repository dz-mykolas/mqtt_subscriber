#include "uci_utils.h"

int utils_load_config(char *cfg, struct uci_context **ctx, struct uci_package **pkg)
{
    *ctx = uci_alloc_context();
    if (!(*ctx))
        return UCI_ERR_MEM;
    uci_set_confdir(*ctx, "/home/studentas/Documents/RUTXXX_R_GPL/openwrt-gpl-ipq40xx-generic.Linux-x86_64/package/mqtt_subscriber/files/");
    if (uci_load(*ctx, cfg, pkg) != UCI_OK) {
        log_event(LOG_EVENT_ERROR, "CFG: Failed to load config file: %s", cfg);
        uci_free_context(*ctx);
        return UCI_ERR_NOTFOUND;
    }
    return UCI_OK;
}

struct topic *create_topic_from_section(struct uci_section *section)
{
    char topic_name[MAX_TOPIC_L] = "";
    int topic_qos = -1;
    struct uci_element *opts;

    uci_foreach_element(&section->options, opts) {
        struct uci_option *o = uci_to_option(opts);
        if (strcmp(o->e.name, "name") == 0)
            snprintf(topic_name, MAX_TOPIC_L, "%s", o->v.string);
        else if (strcmp(o->e.name, "qos") == 0)
            topic_qos = atoi(o->v.string);
    }

    if (strcmp(topic_name, "") == 0 || (topic_qos < 0 || topic_qos > 2))
        return NULL;
    return create_node(topic_name, topic_qos);
}

int utils_get_topics(struct topic **topics_list)
{
    struct uci_context *ctx;
    struct uci_package *pkg;
    struct uci_element *sct;
    if (utils_load_config("mqtt_subscriber_topics.config", &ctx, &pkg) != UCI_OK)
        return 1;
    uci_foreach_element(&pkg->sections, sct) {
        struct uci_section *section = uci_to_section(sct);
        if (strcmp(section->type, "topic") == 0) {
            struct topic *tpc = create_topic_from_section(section);
            if (tpc)
                llist_add_end(topics_list, tpc);
            else
                log_event(LOG_WARNING, "CFG/TOPIC: Could not create topic");
        }
    }
    uci_free_context(ctx);
    return 0;
}

topic_options convert_topic_options(char *option)
{
    if (strcmp(option, "topic") == 0) {
        return EVENT_TOPIC_NAME;
    } else if (strcmp(option, "type") == 0) {
        return EVENT_COMPARISON_TYPE;
    } else if (strcmp(option, "param") == 0) {
        return EVENT_PARAMETER;
    } else if (strcmp(option, "comparator") == 0) {
        return EVENT_COMPARATOR;
    } else if (strcmp(option, "ref_value") == 0) {
        return EVENT_REF_VALUE;
    } else if (strcmp(option, "sender") == 0) {
        return EVENT_SENDER;
    } else if (strcmp(option, "recipient") == 0) {
        return EVENT_RECIPIENT;
    } else {
        return -1;
    }
}

int create_event_from_section_helper(struct event *evt, struct topic **tpc, struct uci_option *o)
{
    bool tpc_exists = false;
    switch (convert_topic_options(o->e.name)) {
        case EVENT_TOPIC_NAME:
            // Can replace with hash table to loop once O(n) and then O(1) check if it exists
            *tpc = find_topic(*tpc, o->v.string);
            tpc_exists = (*tpc != NULL);
            if (!tpc_exists) {
                return 1;
            }
            snprintf(evt->topic_name, MAX_TOPIC_L, "%s", o->v.string);
            return 0;
        case EVENT_COMPARISON_TYPE:
            if (!(strcmp(o->v.string, "0") == 0 || strcmp(o->v.string, "1") == 0)) {
                log_event(LOG_WARNING, "CFG/EVENT: Wrong event comparison type");
                return 1;
            }
            evt->type = atoi(o->v.string);
            return 0;
        case EVENT_PARAMETER:
            snprintf(evt->param, 50, "%s", o->v.string);
            return 0;
        case EVENT_COMPARATOR:
            if (convert_comparator(o->v.string) == -1)
                return 1;
            snprintf(evt->comp_sym, 3, "%s", o->v.string);
            return 0;
        case EVENT_REF_VALUE:
            if (evt->type == TYPE_ALPHANUMERIC)
                snprintf(evt->ref_string, 100, "%s", o->v.string);
            else
                evt->ref_num = atof(o->v.string);
            return 0;
        case EVENT_SENDER:
            snprintf(evt->sender, 100, "%s", o->v.string);
            return 0;
        case EVENT_RECIPIENT:
            if (o->type == UCI_TYPE_LIST) {
                struct uci_element *list_e;
                evt->recipient_count = 0;
                uci_foreach_element(&o->v.list, list_e) {
                    snprintf(evt->recipient[evt->recipient_count], 100, "%s", list_e->name);
                    evt->recipient_count++;
                }
            } else {
                log_event(LOG_WARNING, "CFG/EVENT: Recipient type not a list");
                return 1;
            }
            return 0;
        default: 
            log_event(LOG_WARNING, "CFG/EVENT: Bad config format");
        return 0;
    }
}

struct event create_event_from_section(struct topic **tpc, struct uci_section *section)
{
    struct uci_element *opts;
    struct event evt;
    
    uci_foreach_element(&section->options, opts) {
        struct uci_option *o = uci_to_option(opts);
        if (create_event_from_section_helper(&evt, tpc, o) == 1) {
            evt.topic_name[0] = '\0';
            break;
        }
    }
    return evt;
}

int utils_get_events(struct topic **topics_list)
{
    struct uci_context *ctx;
    struct uci_package *pkg;
    struct uci_element *sct;
    if (utils_load_config("mqtt_subscriber_events.config", &ctx, &pkg) == 1)
        return 1;

    uci_foreach_element(&pkg->sections, sct) {
        struct uci_section *section = uci_to_section(sct);
        if (strcmp(section->type, "event") == 0) {
            struct topic *tpc = *topics_list;
            struct event evt = create_event_from_section(&tpc, section);
            if (evt.topic_name[0] != '\0')
                llist_add_event(tpc, evt);
        }
    }
    uci_free_context(ctx);
    return 0;
}

int utils_get_data(struct topic **topics_list)
{
    utils_get_topics(topics_list);
    utils_get_events(topics_list);
}
