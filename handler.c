#include "globals.h"
#ifdef ENABLE_LOGGING
#include "log.h"
#endif
#include "handler.h"
#include "config.h"
#include "status.h"
#include "strndup.h"
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFSIZE 4096

#if defined(ENABLE_HANDLERS) || defined(ENABLE_CGI)
/** Invoke a handler for a requested object */
void invoke_handler(const char *handler, struct request *req) {
	char *p, *q, *r, *s, *t;

#ifdef ENABLE_LOGGING
	log_request(req);
#endif

	/* Set environment variables */
	setenv("SERVER_PROTOCOL", req->proto, 1);
	setenv("REQUEST_METHOD", req->method, 1);
	setenv("REQUEST_URI", req->uri, 1);
	setenv("REMOTE_ADDR", inet_ntoa(req->remote_addr.sin_addr), 1);
	p = strchr(req->uri, '?');
	if(p) {
		setenv("SCRIPT_NAME", strndup(req->uri, p - req->uri), 1);
		setenv("QUERY_STRING", p + 1, 1);
	} else {
		setenv("SCRIPT_NAME", req->uri, 1);
	}
	p = malloc(strlen(config->webdir) + strlen(req->filename) + 1);
	if(!p) {
		req->filename = message_file[HTTP_500];
		req->status = HTTP_500;
		return handle_request(req);
	}
	sprintf(p, "%s%s", config->webdir, req->filename);
	setenv("SCRIPT_FILENAME", p, 1);
	setenv("REDIRECT_STATUS", message[req->status], 1);

	/* Transform posted headers into environment variables */
	p = req->buf;
	for(;;) {
		q = strpbrk(p, "\r\n"); /* Find end of line */
		if(q == p) break; /* Empty line, we're done */

		/* Zero *q, move q past end of line */
		q++;
		if((*q == '\r' || *q == '\n') && *q != *(q - 1)) {
			*(q - 1) = 0;
			q++;
		} else *(q - 1) = 0;

		/* Detect special headers */
		if(!strncmp(p, "Content-Type: ", 14)) {
			setenv("CONTENT_TYPE", strdup(&p[14]), 1);
		} else if(!strncmp(p, "Content-Length: ", 16)) {
			setenv("CONTENT_LENGTH", strdup(&p[16]), 1);
		} else if(r = strstr(p, ": ")) {
			/* Normal headers get HTTP_ prepended */
			s = malloc(r - p + 6);
			strncpy(s, "HTTP_", 5);
			strncpy(s + 5, p, r - p);
			s[r - p + 5] = 0;
			/* Transform variable name */
			for(t = s + 5; *t; t++) {
				if(*t == '-') *t = '_';
				else *t = toupper(*t);
			}
			setenv(s, r + 2, 1);
		}
		/* On to the next line */
		p = q;
	}

	/* Change directory */
	p = strdup(&req->filename[1]);
	chdir(dirname(p));
	free(p);

	/* Get filename part */
	p = strdup(req->filename);

	/* Exec handler */
	printf("HTTP/1.1 %s\r\nConnection: close\r\n",
		message[req->status]);
	fflush(stdout);
#if 0
	if(!strcmp(req->method, "POST")) {
		fputs("POST Request will trash server!\n", stderr);
	}
#endif
	if(handler) execl(handler, handler, basename(p), NULL);
#ifdef ENABLE_CGI
	else execl(getenv("SCRIPT_FILENAME"), basename(p), NULL);
#endif /* ENABLE_CGI */
	free(p);

	/* Send internal server error */
	chdir(config->webdir);
	req->filename = message_file[HTTP_500];
	req->status = HTTP_500;
	return handle_request(req);
}
#endif /* ENABLE_HANDLERS || ENABLE_CGI */

#ifdef ENABLE_CGI
/** Run a CGI script */
void run_cgi(struct request *req) {
	return invoke_handler(NULL, req);
}
#endif /* ENABLE_CGI */
