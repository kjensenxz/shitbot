/*
 * net.c
 *
 * Shitbot is licensed under the GNU GPL, version 3 or later.
 * See the LICENSE.
 * Copyright (C) Kenneth B. Jensen 2021. All rights reserved.
 *
 */
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

void *get_in_addr(struct sockaddr *);
int net_connect(char *, char *);
int net_close(int);

/* Thanks, Beej! */
void
*get_in_addr(struct sockaddr *sa) {
	if (AF_INET == sa->sa_family) {
		return 
		   &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int
net_connect(char *host, char *service) {
	int sock = -1;

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *svinfo = NULL, *p = NULL;

	int rv = -1;
	char s[INET6_ADDRSTRLEN] = {0};


	rv = getaddrinfo(host, service, &hints, &svinfo);
	if (0 != rv) {
		printf("getaddrinfo: %s in net_connect.\n",
		      gai_strerror(rv));
		return -1;
	}

	for (p = svinfo; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype,
			     p->ai_protocol);
		if (0 > sock) {
			perror("warning: socket failed "
			      "in net_connect");
			continue;
		}
		rv = connect(sock, p->ai_addr, 
		            p->ai_addrlen);
		if (0 > rv) {
			perror("warning: connect failed "
			      "in net_connect");
			close(sock);
			continue;
		}

		break;
	}

	if (p == NULL) {
		printf("Failed to connect in net_connect."
		      "\n");

		freeaddrinfo(svinfo);
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr(
		     (struct sockaddr *)p->ai_addr
		 ), s, sizeof s);
	printf("connecting to %s\n", s);

	freeaddrinfo(svinfo);

	return sock;
}

int
net_close(int sock) {
	return close(sock);
}

/* 
 * tdprintf() is like dprintf(3) for use on a socket file descriptor and tees to
 * STDOUT_FILENO.
 */
int tdprintf(int fd, const char *fmt, ...) {
	va_list ap;
	char *resp = NULL;
	int respc = -1;
	int rv = -1;

	/* sanity check! */
	if (NULL == fmt || 0 > fd) {
		return -1;
	}
	
	va_start(ap, fmt);
	respc = vasprintf(&resp, fmt, ap);
	va_end(ap);

	if (0 > respc) {
		perror("vasprintf() in tdprintf");
		goto _tdprintf_error;
	}

	rv = send(fd, resp, respc, 0);
	if (0 > rv) {
		perror("send() in tdprintf");
		goto _tdprintf_error;
	}
	
	rv = write(STDOUT_FILENO, resp, respc);
	if (0 > rv) {
		perror("write() in tdprintf");
		goto _tdprintf_error;
	}

	free(resp);
	return respc;

_tdprintf_error:
	if (NULL != resp) {
		free(resp);
	}
	return -1;
}
