#include "uci_utils.h"

int utils_get_topics(struct topic **topics_list)
{
    struct uci_context *ctx;
    struct uci_package *pkg;
    struct uci_element *sct;
    ctx = uci_alloc_context();
    if (!ctx) {
        return 1;
    }
    uci_set_confdir(ctx, "/home/studentas/Documents/RUTXXX_R_GPL/openwrt-gpl-ipq40xx-generic.Linux-x86_64/package/mqtt_subscriber/files/");
    if (uci_load(ctx, "mqtt_subscriber.config", &pkg) != UCI_OK) {
        log_event(LOG_EVENT_ERROR, "Failed to load UCI config file.");
        uci_free_context(ctx);
        return 1;
    }
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
                else
                    topic_qos = atoi(o->v.string);
            }

            struct topic *tpc = create_node(topic_name, topic_qos);
            llist_add_end(topics_list, tpc);
        }
    }
    uci_free_context(ctx);
    return 0;
}

int utils_get_events(struct event **topics_list)
{

}