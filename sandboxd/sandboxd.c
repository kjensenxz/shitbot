#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "net.h"
#include "util.h"

#define strlncasecmp(BIG, LITTLE) strncasecmp(BIG, LITTLE, strlen(LITTLE))

/* 
 * to hold our user input in the format
 * <command> <space> <argument> <newline>
 */
typedef struct instruction {
	char *cmd;
	char *arg;
} instruction_t;

instruction_t *get_instruction(char *);
char *dispatch(instruction_t *);
char *get_console_port(void);
char *get_qmp_port(void);

char *get_console_port(void) {
	return strdup("6666");
}
char *get_qmp_port(void) {
	return strdup("7777");
}

/* 
 * use of a struct is in hopes of extensibility without having to change
 * function signatures.
 */
instruction_t *
get_instruction(char *line) {
	char *space = NULL;
	char *newline = NULL;

	instruction_t *instruction = NULL;

	instruction = calloc(1, sizeof(instruction_t));
	if (NULL == instruction) {
		ERR("Unable to allocate memory in get_instruction: %s", 
			strerror(errno));
		return NULL;
	}

	/* to be allocated */
	instruction->cmd = NULL;
	instruction->arg = NULL;

	/* 
	 * input should have two words:
	 * 	<command> <space> <argument> <newline>
	 * e.g.:
	 * 	start minimal_image
	 * 	stop 1111
	 */
	space = index(line, ' ');
	if (NULL == space) {
		ERR("Unable to parse client data.\n");
		return NULL;
	}
	
	/* space-line -> length of first word (cmd) */
	instruction->cmd = strndup(line, space-line);
	if (NULL == instruction->cmd) {
		ERR("Unable to parse client data.\n");
		return NULL;
	}
	
	/* 
	 * read at offset: space+1 to end-of-line -> second word
	 * index(line, \n)-space-1 -> second word length without newline (arg)
	 */
	newline = index(line, '\n');
	instruction->arg = strndup(&line[space-line+1], newline-space-1);
	if (NULL == instruction->arg) {
		ERR("Unable to parse client data.\n");
		return NULL;
	}

	return instruction;
}

char *
dispatch(instruction_t *instruction) {
	char *const cmd = instruction->cmd;

	const char *sandbox_path = 
		"/home/kbjensen/prog/sandbox/src/sandbox";

	char **argv = NULL;
	int start = 0, reset = 0;
	char *console_port = NULL;
	
	pid_t pid = {0};
	int i = -1;
	char *resp = NULL;

	/* reusable comparison variables */
	start = (0 == strlncasecmp(cmd, "start"));
	reset = (0 == strlncasecmp(cmd, "reset"));
	if (start || reset) {
		 /* <start|reset> <config_name> */
		 /* 6 <- path, start, config_name, qmp_port, console_port, NULL */
		argv = calloc(6, sizeof(char *));
		/* omitting argv[0], since the binary is the same for each */

		console_port = get_console_port();

		argv[2] = strdup(instruction->arg);
		argv[3] = get_qmp_port();
		argv[4] = console_port;
		argv[5] = NULL;
		
		/* 
		 * setting argv[1] last to register with psql with single 
		 * branch.
		 */
		if (start) {
			argv[1] = strdup("start");
/* TODO: psql
			pg_register_vm(argv);
*/
		}
		else if (reset) {
			argv[1] = strdup("reset");
/* TODO: psql			
			pg_delete_vm(console_port);
			pg_register_vm(argv);
*/
		}
		else {
			assert(0);
		}
	}
	else if (0 == strlncasecmp(cmd, "stop")) {
		/* stop <console_port> */
		/* 4 <- path, stop, qmp_port, NULL */
		argv = calloc(4, sizeof(char *));

		console_port = instruction->arg;

		argv[1] = strdup("stop");
		argv[2] = console_port;
		argv[3] = NULL;
/* TODO: psql
		pg_delete_vm(console_port);
*/
	}
	else {
		assert(0); // bad command
	}

	argv[0] = strdup(sandbox_path);
	pid = fork();

	if (0 == pid) { // child
		execv(sandbox_path, argv);
	}
	else {
		printf("started pid %d with:", pid);
		
		for (i = 0; argv[i] != NULL; ++i) {
			printf(" %s", argv[i]);
		}
		printf("\n");

		free(argv[0]);
		free(argv[1]);
		free(argv[2]);

		if (reset || start) {
			free(argv[3]);
			free(argv[4]);
		}
		free(argv);

		asprintf(&resp, "%d", pid);
		return resp;
	}

	assert(0);
	return NULL; // wtf?
}

int
child_worker(int clientfd) {
	char *line = NULL;
	char *resp = NULL;
	ssize_t rv = 0;

	instruction_t *instruction = NULL;

	dprintf(clientfd, "sandbox indev2020-08-21 ready\n");

	line = calloc(256, sizeof(char));
	// XXX check

	rv = recv(clientfd, line, 255, 0);
	if (0 > rv) {
		perror("Failed to read from client:");
		return -1;
	}
	else if (0 == rv) {
		ERR("No data recieved from client.");
		return -1;
	}
	DEBUG("%s\n", line);

	instruction = get_instruction(line);

	resp = dispatch(instruction);
	if (NULL == resp) {
		ERR("Unsuccessful dispatch.");
		dprintf(clientfd, "Error handling request.");
		return -1;
	}

	dprintf(clientfd, "pid %s started. goodbye.\n", resp);

	
	printf("%s\n", instruction->cmd);
	printf("%s\n", instruction->arg);

	free(instruction->cmd);
	free(instruction->arg);
	free(instruction);
	free(line);
	free(resp);
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv) {
	int serv_sock = -1;
	int clnt_sock = -1;
	pid_t pid = 0;
	int i = 0;

//      TODO: for running in background, will be under a flag
//	pid = fork();
//	if (0 != pid) {
//		exit(0);
//	}

	serv_sock = net_sslisten("localhost", "1234");

	while (1) {
		clnt_sock = net_accept(serv_sock); // we got a live one
		++i;

		pid = fork();
		if (0 == pid) { // child
			close(serv_sock);
			child_worker(clnt_sock);

			close(clnt_sock);
			exit(0);
		}
		else { // parent
			close(clnt_sock);
			wait(NULL);
			printf("pid %d terminated.\n", pid);
		}
		close(clnt_sock);
	}
	return 0;
}
