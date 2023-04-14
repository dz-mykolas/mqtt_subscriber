// #include "curl_utils.h"

// static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
//     struct email_payload *payload = (struct email_payload *)userp;
//     const char **recipients = payload->recipients;

//     char *formatted_recipients = NULL;
//     size_t total_len = 0;

//     while (*recipients) {
//         const char *to = "To: ";
//         size_t len = strlen(to) + strlen(*recipients) + strlen("\r\n");
//         total_len += len;

//         if (formatted_recipients == NULL) {
//             formatted_recipients = malloc(len + 1);
//             snprintf(formatted_recipients, len + 1, "%s%s\r\n", to, *recipients);
//         } else {
//             formatted_recipients = realloc(formatted_recipients, total_len + 1);
//             snprintf(formatted_recipients + total_len - len, len + 1, "%s%s\r\n", to, *recipients);
//         }

//         recipients++;
//     }

//     const char *data_template[] = {
//         "Date: Mon, 29 Jul 2023 09:26:29 +0200\r\n",
//         formatted_recipients,
//         "From: sender@example.com\r\n",
//         "Subject: Test email using libcurl in C\r\n",
//         "\r\n", /* empty line to divide headers from the body */
//         "This is the body of the email.\r\n",
//         NULL
//     };

//     const char *payload_data;

//     if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
//         return 0;
//     }

//     payload_data = data_template[payload->lines_read];

//     if (payload_data) {
//         size_t len = strlen(payload_data);
//         memcpy(ptr, payload_data, len);
//         payload->lines_read++;
//         return len;
//     }

//     free(formatted_recipients);
//     return 0;
// }