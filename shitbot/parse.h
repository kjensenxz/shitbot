#include <stddef.h>
typedef struct irc_message {
	char *prefix;
	char *nick;
	char *ident;
	char *host;
	char *command;
	char *middle;
	char *params;
	char *trailing;
} ircmsg_t;

ircmsg_t * parseline(char *line, size_t n);
