#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

struct email_payload {
	int lines_read;
	const char **recipients;
};