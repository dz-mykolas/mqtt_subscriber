#include "uci_utils.h"

int utils_load_config(char *cfg, struct uci_context **ctx, struct uci_package **pkg)
{
    *ctx = uci_alloc_context();
    if (!(*ctx))
        return 1;
    uci_set_confdir(*ctx, "/home/studentas/Documents/RUTXXX_R_GPL/openwrt-gpl-ipq40xx-generic.Linux-x86_64/package/mqtt_subscriber/files/");
    if (uci_load(*ctx, cfg, pkg) != UCI_OK) {
        log_event(LOG_EVENT_ERROR, "Failed to load UCI config file.");
        uci_free_context(*ctx);
        return 1;
    }
    return 0;
}

int utils_get_topics(struct topic **topics_list)
{
    struct uci_context *ctx;
    struct uci_package *pkg;
    struct uci_element *sct;
    utils_load_config("mqtt_subscriber_topics.config", &ctx, &pkg);
    uci_foreach_element(&pkg->sections, sct) {
        struct uci_section *section = uci_to_section(sct);
        if (strcmp(section->type, "topic") == 0) {
            struct uci_element *opts;
            char topic_name[MAX_TOPIC_L];
            int topic_qos;
            uci_foreach_element(&section->options, opts) {
                struct uci_option *o = uci_to_option(opts);
                int i = 0;
                if (strcmp(o->e.name, "name") == 0)
                    snprintf(topic_name, MAX_TOPIC_L, "%s", o->v.string);
                else if (strcmp(o->e.name, "qos") == 0)
                    topic_qos = atoi(o->v.string);
            }
            struct topic *tpc = create_node(topic_name, topic_qos);
            llist_add_end(topics_list, tpc);
        }
    }
    uci_free_context(ctx);
    return 0;
}

int utils_get_events(struct topic **topics_list)
{
    struct uci_context *ctx;
    struct uci_package *pkg;
    struct uci_element *sct;
    utils_load_config("mqtt_subscriber_events.config", &ctx, &pkg);
    uci_foreach_element(&pkg->sections, sct) {
        struct uci_section *section = uci_to_section(sct);
        if (strcmp(section->type, "event") == 0) {
            struct uci_element *opts;
            struct topic *tpc = NULL;
            struct event evt;
            bool tpc_exists = false;
            uci_foreach_element(&section->options, opts) {
                struct uci_option *o = uci_to_option(opts);
                if (strcmp(o->e.name, "topic") == 0) {
                    snprintf(evt.topic_name, MAX_TOPIC_L, "%s", o->v.string);
                    // Can replace with hash table to loop once O(n) and then O(1) check if it exists
                    tpc = find_topic(*topics_list, evt.topic_name);
                    tpc_exists = (tpc != NULL);
                    if (!tpc_exists)
                        break;
                }
                else if (strcmp(o->e.name, "param") == 0)
                    snprintf(evt.param, 50, "%s", o->v.string);
                else if (strcmp(o->e.name, "comparison") == 0)
                    snprintf(evt.comp_sym, 3, "%s", o->v.string);
                else if (strcmp(o->e.name, "ref_value") == 0)
                    evt.ref = atoi(o->v.string);
                else if (strcmp(o->e.name, "sender") == 0)
                    snprintf(evt.sender, 100, "%s", o->v.string);
                else if (strcmp(o->e.name, "recipient") == 0 && o->type == UCI_TYPE_LIST) {
                    struct uci_element *list_e;
                    evt.recipient_count = 0;
                    uci_foreach_element(&o->v.list, list_e) {
                        snprintf(evt.recipient[evt.recipient_count], 100, "%s", list_e->name);
                        evt.recipient_count++;
                    }
                }
            }
            if (tpc_exists) {
                llist_add_event(tpc, evt);
            }
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
