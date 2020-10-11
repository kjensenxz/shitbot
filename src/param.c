/*
 * param.c
 *
 * Sandbox is licensed under the GNU GPL, version 3 or later.
 * See the LICENSE.
 * Copyright (C) Kenneth B. Jensen 2021. All rights reserved.
 *
 * This file was developed with creating parameters for
 * minimal_image in mind.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sandbox.h"
#include "param.h"

/* configuration for mini-images */
const char *DEFAULT_PARAMS[] = {
	QEMU_SYSTEM_BIN,
	"-no-user-config",
	"-sandbox",
	"on,obsolete=deny,elevateprivileges=deny,"
		"spawn=deny,resourcecontrol=deny",
	"-nodefaults",
	"-nographic",
	"-no-reboot",
	"-no-shutdown",
	"-m", "64M",
	NULL
};

/* 
 * designed for use with getline to remove the trailing newline.
 * ex: X  Y  Z  A  B  C  \n \0
 *     0  1  2  3  4  5  6  7
 *     strlen() -> 7
 *     Change #6 to \0, just cause it works.
 * Slightly dangerous.
 */
#define chop(STR) STR[strlen(STR)-1] = '\0'

/* designed for get_defaultparams to reduce code smell */
#define growarray(ARRAY, COUNTER, SIZE)                    \
	do {                                               \
		++COUNTER;                                 \
		ARRAY = realloc(ARRAY, COUNTER * SIZE);    \
		if (NULL == ARRAY) {                       \
			printf("fatal: realloc returned"   \
			       "NULL in get_defaultparams" \
			       "\n"                        \
			);                                 \
		}                                          \
	} while (0);                                       \

/* 
 * This smelly pile of code creates a str array
 * for use with exec from a config file (config_name).
 * Most of it needs to be subbed out to a subroutine.
 */
char **
get_startparams(char *config_name, char *qmp_port,
                int console_port) {
	FILE *config_file = NULL;
	char **params = NULL;
	size_t nparam = 0;

	/* Templates for the template god! */
	char *const console_template = "socket,id=serio,"
		"port=%d,host=localhost,server,nowait";
//		",telnet";
	char *const qmp_template =
		"tcp:localhost:%s,server,nowait";

	/* unsigned so GCC will shut the fuck up */
	unsigned int i = 0;
	char *line = NULL;
	size_t count = 0;
	/* to silence gcc complaining about casting */
	const char *str = NULL;

	config_file = fopen(config_name, "r");

	if (NULL == config_file) {
		printf("Could not open config file %s.\n", 
		       config_name);
		return NULL;
	}

	/*
	 * There's probably a cleaner way to do this.
	 * This needs to be moved out into a subroutine.
	 * However, when moving it, I ran into a problem 
	 * with the function returning a null pointer.
	 * Anyways, load up the default params...
	 */
	for (i = 0, str = DEFAULT_PARAMS[i]; 
	     str != NULL; 
	     ++i, str = DEFAULT_PARAMS[i]) {
		growarray(params, nparam, sizeof(char*))
		params[nparam-1] = strdup(str);
	}

	/* Add the specific config-based params... */
	while (0 < getline(&line, &count, config_file)) {
		growarray(params, nparam, sizeof(char*))

		/* getline() returns line endings; we
		 * gotta chomp it. */
		chop(line);
		params[nparam-1] = strdup(line);
	}

	/* Add some magic beans... */
	growarray(params, nparam, sizeof(char*))
	params[nparam-1] = strdup("-chardev");

	growarray(params, nparam, sizeof(char*))
	asprintf(&params[nparam-1], console_template, 
	         console_port);

	/* a little repetitive... */
	growarray(params, nparam, sizeof(char*))
	params[nparam-1] = strdup("-qmp");

	growarray(params, nparam, sizeof(char*))
	asprintf(&params[nparam-1], qmp_template, 
	         qmp_port);
	
	/* 
	 * exec() expects its array to be 
	 * NULL-terminated. 
	 */
	growarray(params, nparam, sizeof(char*))
	params[nparam-1] = NULL;

	/* and sanity check. */
	for (i = 0; i < nparam; ++i) {
		if (NULL == params[i]) {
			printf("(null) ");
			continue;
		}
		printf("%s ", params[i]);

	}
	printf("\n");

	return params;
}
