#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "net.h"
#include "parse.h"
#include "util.h"


#define strlncmp(BIG, LITTLE) strncmp(BIG, LITTLE, strlen(LITTLE))

#define checkfree(PTR)                  \
	do {                            \
		if (NULL != PTR) {      \
			free(PTR);      \
		}                       \
		PTR = NULL;             \
	} while(0);

_Noreturn void usage(int, char *);
void freeircmsg(ircmsg_t *);
void sandbox_worker(int, char*);
int tdprintf(int, const char *, ...);

_Noreturn void
usage(int exitcode, char *argv0) {
	printf("%s <irc.server.net> <portno>\n", argv0);
	exit(exitcode);
}

void
freeircmsg(ircmsg_t *msg) {
	checkfree(msg->prefix);
	checkfree(msg->nick);
	checkfree(msg->ident);
	checkfree(msg->host);
	checkfree(msg->command);
	checkfree(msg->middle);
	checkfree(msg->params);
	checkfree(msg->trailing);
	checkfree(msg);
}

void
sandbox_worker(int pipefd, char *cmdline) {
	int sandbox_sock = -1;
	char buf[256] = {0};
	ssize_t rv = -1;

	sandbox_sock = net_connect("localhost", "1111");
	if (0 > sandbox_sock) {
		return;
	}
	/* 
	 * run command line stored in argv
	 */
	dprintf(sandbox_sock, "%s\r\n", cmdline);

	/*
	 * gather output from command, send to parent via pipe
	 */
	while(0 < (rv = recv(sandbox_sock, buf, 255, 0))) {
		write(pipefd, buf, rv);
		memset(buf, 0, 256);
	}
	exit(0);
	close(pipefd);
	return;
}



void
dispatch(int irc, ircmsg_t *msg) {
	char *resp = NULL;
	size_t respc = -1;
	if (0 == strlncmp(msg->command, "NOTICE")) {
		if (NULL != strstr(msg->params, "Found your hostname") ||
		    NULL != strstr(msg->params, "Couldn't look up your hostname")) {
			tdprintf(irc, "NICK shitbot\r\n" "USER shitbot * * :shitbot\r\n");
		}
	}
	if (0 == strlncmp(msg->command, "PING")) {
		tdprintf(irc, "PONG %s\r\n", msg->params);
	}
	/*
	 * RPL_MYINFO; indicates successful registration as per RFC2812
	 */
	if (0 == strlncmp(msg->command, "004")) {
		tdprintf(irc, "JOIN #homescreen\r\n");
	}
	if (0 == strlncmp(msg->command, "INVITE")) {
		tdprintf(irc, "JOIN %s\r\n", msg->params);
	}
	if (0 == strlncmp(msg->command, "PRIVMSG") && 
	    0 != strlncmp(msg->middle, "shitbot")) {
		if (0 == strlncmp(msg->params, ".bots")) {
			tdprintf(irc, "PRIVMSG %s :Reporting in! [GNU17 C]\r\n", msg->middle);
		}

		if (0 == strlncmp(msg->params, "#! ")) {
			int pipefd[2] = {-1};
			pipe2(pipefd, O_NONBLOCK);

			pid_t pid = -1;
			pid = fork();
			if (0 > pid) {
				tdprintf(irc,"PRIVMSG %s :Unable to fork()! Fuck!\r\n", msg->middle);
				perror("fork()");
				close(pipefd[0]);
				close(pipefd[1]);
			}
			else if (0 == pid) {
				close(irc);
				close(pipefd[0]);
				sandbox_worker(pipefd[1], &msg->params[3]);
				freeircmsg(msg);
				close(pipefd[1]);
				exit(0);
			}
			else {
				int wstatus = 0;
				int rv = -1;
				close(pipefd[1]);

				char buf[256] = {0};
				char *resp = NULL;
				int respc = 0;
				struct timespec time1 = {0}, time2 = {0};
				printf("pid %d started.\n", pid);
				clock_gettime(CLOCK_MONOTONIC_RAW, &time1);
				// XXX: check clock_gettime
				do {
					clock_gettime(CLOCK_MONOTONIC_RAW, &time2);
					if (time2.tv_sec - time1.tv_sec > 3) {
						printf("test\n");
						if (0 > kill(pid, SIGKILL)) {
							printf("killing %d failed: %s\n",
								pid, strerror(errno));
						}
						break;
					}
					rv = read(pipefd[0], buf, 255);
					if (0 >= rv && EWOULDBLOCK != errno) {
						perror("read");
						break;
					}
					if ('#' == buf[0]) {
						kill(pid, SIGKILL);
						break;
					}
					if (0 < rv) {
						respc += rv;
						resp = realloc(resp, respc);
						if (NULL == resp) {
							perror("realloc");
							break;
						}
						memcpy(&resp[respc-rv], buf, rv);
					}

					rv = waitpid(pid, &wstatus, WNOHANG);
					if (0 > rv) {
						perror("waitpid for sandbox_worker child");
						break;
					}
					if (pid == rv) {
						printf("pid %d exited.\n", pid);
						break;
					}

				} while(1);
				close(pipefd[0]);

				char *line = NULL;
				char *save = NULL;
				line = strtok(resp, "\r\n");
				while (NULL != (line = strtok(NULL, "\r\n"))) {
					tdprintf(irc, "PRIVMSG %s :%s\r\n", 
						msg->middle, line);
					usleep(300000);
				}
				free(resp);
			}
		}
	}
}
int
main(int argc, char **argv) {
	int irc = -1;

	if (3 > argc) {
		usage(EXIT_FAILURE, argv[0]);
	}

	irc = net_connect(argv[1], argv[2]);
	if (0 > irc) {
		return EXIT_FAILURE;
	}

	#define MSGMAXLEN 512
	char buf[MSGMAXLEN+1] = {0};
	ssize_t n = 0;

	ircmsg_t *msg = NULL;


	//buf = calloc(MSGMAXLEN, sizeof(char));
	while (0 < (n = recv(irc, buf, MSGMAXLEN, 0))) {
		for (size_t i = 0, offset = 0; n > i; ++i) {
			if ('\n' == buf[i]) {
				write(1, &buf[offset], i-offset+1);
				msg = parseline(&buf[offset], i-offset+1);
				if (NULL == msg) {
					goto _main_error;
				}
				offset = i+1;
				dispatch(irc, msg);
				if (NULL != msg){
					freeircmsg(msg);
				}
			}
		}

		memset(buf, '\0', MSGMAXLEN);
	}

_main_error:
	if (NULL != msg) {
		freeircmsg(msg);
	}
	if (0 > n) {
		perror("recv");
	}
	free(buf);

	close(irc);

	return 0;
}
