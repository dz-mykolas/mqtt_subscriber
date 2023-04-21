#include "curl_utils.h"

int send_mail(struct event evt, struct email_node *eml, char *ref_value)
{
	CURL *curl;
	CURLcode err_curl;
	struct curl_slist *recipients = NULL;

	char *sender	    = eml->sender;
	char *smtp_username = eml->smtp_username;
	char *smtp_userpass = eml->smtp_userpass;
	char *smtp_ip	    = eml->smtp_ip;
	int smtp_port	    = eml->smtp_port;
	char *event_topic   = evt.topic_name;
	char smtp_url[128];
	int ref_len = strlen(ref_value);
	snprintf(smtp_url, 128, "smtps://%s:%d", smtp_ip, smtp_port);

    if (evt.recipient_count < 1) {
        log_event(LOG_WARNING, "MSG: No recipients");
        return 1;
    }
	for (int i = 0; i < evt.recipient_count; i++)
		recipients = curl_slist_append(recipients, evt.recipients[i]);

	curl = curl_easy_init();
	if (!curl) {
		log_event(LOG_EVENT_ERROR, "Could not initialize curl object");
		return 1;
	}

	curl_mime *mime;
	curl_mimepart *part;

    /* Header */
    struct curl_slist *headers = NULL;
    char header_text[MAX_SUBJECT_SIZE];
    snprintf(header_text, sizeof(header_text), "Subject: %s", event_topic);
    headers = curl_slist_append(headers, header_text);
    
	/* Payload */
    char payload_text[MAX_PAYLOAD_SIZE];
    snprintf(payload_text, sizeof(payload_text), "%.*s", ref_len,
            ref_value);
    mime = curl_mime_init(curl);
	part = curl_mime_addpart(mime);
	curl_mime_data(part, payload_text, CURL_ZERO_TERMINATED);
	curl_mime_type(part, "text/plain");
	curl_mime_encoder(part, "7bit");
	curl_mime_headers(part, curl_slist_append(NULL, "Content-Disposition: inline"), 1);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_username);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_userpass);
	curl_easy_setopt(curl, CURLOPT_URL, smtp_url);
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sender);
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

	err_curl = curl_easy_perform(curl);
	if (err_curl != CURLE_OK)
		log_event(LOG_EVENT_ERROR, "Failed to send a message: %s\n", curl_easy_strerror(err_curl));
	log_event(LOG_EVENT_ERROR, "Sent a message");
	curl_slist_free_all(recipients);
	curl_mime_free(mime);
	curl_easy_cleanup(curl);
	return 0;
}