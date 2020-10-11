/*
 * sandbox.c
 *
 * Sandbox is licensed under the GNU GPL, version 3 or later.
 * See the LICENSE.
 * Copyright (C) Kenneth B. Jensen 2021. All rights reserved.
 *
 * This program is a prototype implementation of the RFC
 * contained in the root directory.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sandbox.h"
#include "param.h"
#include "net.h"

/* just for comparing strings a little safer */
#define strlncmp(BIG, LITTLE) strncmp(BIG, LITTLE, strlen(LITTLE))


/* To-dos:
 * - make sandbox_start free the char ** it gets from get_startparams
 *   & do error checking for execvp not running
 */

_Noreturn void usage(char *, int);
int sandbox_start(char *, char *, int);
int sandbox_stop(char *);
int sandbox_reset(char *, char *, int);

_Noreturn void
usage(char *argv0, int exitcode) {
	printf(
	"%s start <config_name> <qmp_port> <console_port>\n"
	"%s reset <config_name> <qmp_port> <console_port>\n"
	"%s stop <qmp_port>\n"
	"It is in error to call reset without a running instance.\n"
	"This program is free software, licensed under the GNU GPL, version 3 or later. Copyright kjensenxz 2021.\n",
	argv0, argv0, argv0
	);

	exit(exitcode);
}


#define nwrite(FD, STR) write(FD, STR, strlen(STR))
#define BUFLEN 256
int
sandbox_stop(char *qmp_port) {
	int sock = -1;
	int flag = 1;

	char buf[BUFLEN] = {0};
	char *const quit_msg =
		" { \"execute\": \"qmp_capabilities\" } "
		" { \"execute\": \"quit\" } ";

	sock = net_connect("localhost", qmp_port);

	if (-1 == sock) {
		return EXIT_FAILURE;
	}

	while (read(sock, buf, BUFLEN-1)) {
		printf("%s", buf);
		if (flag) {
			flag = 0;
			printf(quit_msg);
			nwrite(sock, quit_msg);
		}
	}
	printf("\n");

	net_close(sock);

	return EXIT_SUCCESS;
}


int
sandbox_start(char *config_name, char *qmp_port, 
              int console_port) {
	char **params = NULL;
	int i = -1;

	/* because building string arrays is messy. */
	params = get_startparams(
		config_name, 
		qmp_port, 
		console_port
	);

	if (NULL == params) {
		printf("get_startparams returned NULL in "
		       "sandbox_start.\n");
		return EXIT_FAILURE;
	}

	for (i = 0; NULL != params[i]; ++i) {
		printf("%s ", params[i]);
	}
	printf("\n");

	execvp(QEMU_SYSTEM_BIN, params);

	/* XXX: doesn't free the char *'s, doesn't really 
	 * matter if the execvp worked*/

	for (i = 0; NULL != params[i]; ++i) {
		free(params[i]);
	}
	free(params);
	
	return EXIT_SUCCESS;
}

int
sandbox_reset(char *config_name, char *qmp_port, 
              int console_port) {
	int rv = -1;

	rv = sandbox_stop(qmp_port);
	if (EXIT_SUCCESS != rv) {
		return EXIT_FAILURE;
	}

	rv = sandbox_start(config_name, qmp_port,
	                   console_port);
	return rv;
}

int
main(int argc, char **argv) {
	char *cmd = NULL;
	char *config_name = NULL;
	char *qmp_port = NULL; /* char* for getaddrinfo */
	int console_port = -1;

	if (argc < 3) {
		usage(argv[0], EXIT_FAILURE);
	}

	cmd = argv[1];

	/* 
	 * check and save strcmp so it doesn't have to be 
	 * done again; for brevity and deduplication.
	 * kinda kludgy :) 
	 */
	int start = -1, reset = -1;
	
	
	start = (0 == strlncmp(cmd, "start"));
        reset = (0 == strlncmp(cmd, "reset"));

	if (start || reset) {
		assert(start != reset);
		/* if one of these isn't true, WTF? */

		/* 
		 * make sure we got enough parameters or 
		 * otherwise we'll start getting into 
		 * something else's memory...
		 */
		if (argc < 5) {
			usage(argv[0], EXIT_FAILURE);
		}
		
		/* XXX: atoi() does not do error checking */
		config_name = argv[2];
		qmp_port = argv[3];
		console_port = atoi(argv[4]);
	
		/* 
		 * reset just calls stop then start... but 
		 * that's outside of this function's job
		 * description. fucking unions.
		 */
		if (start) {
			sandbox_start(
				config_name, 
				qmp_port,
				console_port
			);
		}
		if (reset) {
			sandbox_reset(
				config_name,
				qmp_port,
				console_port
			);
		}
	}
	/* so much easier! */
	else if (0 == strlncmp(cmd, "stop")) {
		/* 
		 * we already checked argc to be >= 3,
		 * not doing it again.
		 */

		qmp_port = argv[2];

		sandbox_stop(qmp_port);
	}

	else {
		usage(argv[0], EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
