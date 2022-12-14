#ifndef REQUEST_H
#define REQUEST_H
#include <sys/types.h>
#include <netinet/in.h>

struct request {
	char *buf;
	char *proto;
	int status;
	char *method;
	char *uri;
	char *filename;
	char *location;
	struct file_type *file_type;
	struct sockaddr_in remote_addr;
};

void do_request(struct sockaddr *addr, socklen_t salen);
void handle_request(struct request *req);
void handle_and_log_request(struct request *req);

#endif /* REQUEST_H */
