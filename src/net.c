/*
 * net.c
 *
 * Sandbox is licensed under the GNU GPL, version 3 or later.
 * See the LICENSE.
 * Copyright (C) Kenneth B. Jensen 2021. All rights reserved.
 *
 */
#include <errno.h>
#include <netdb.h>
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
