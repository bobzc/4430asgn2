typedef struct{
	char *request_method;
	char *url;
	char *host;
	char proxy_conn; // bool
	char connection; // bool
	char cache_ctrl; // bool
	long IMS; // UNIX time for if-modified-since
} http_header;
