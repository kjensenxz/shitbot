#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
ircmsg_t *
parseline(char *line, size_t n) {
	ircmsg_t *msg = NULL;

	/* for saving our place */
	char *token_begin = NULL;
	char *token_end = NULL;
	/* for strn functions */
	size_t token_len = 0;

	/* sanity check */
	if (NULL == line || 2 > n) {
		printf("parseline sanity check fail: %ud: '%s'\n", n, line);
		return NULL;
	}

	/* msg will hold all of the shit we pull apart */
	msg = calloc(1, sizeof(ircmsg_t));
	if (NULL == msg) {
		perror("could not allocate ircmsg_t in parseline");
		return NULL;
	}
	/*
	 * <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
	 * <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
	 * <command>  ::= <letter> { <letter> } | <number> <number> <number>
	 * <SPACE>    ::= ' ' { ' ' }
	 * <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
	 * <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
	 *                or NUL or CR or LF, the first of which may not be ':'>
	 * <trailing> ::= <Any, possibly *empty*, sequence of octets not including
	 *                  NUL or CR or LF>
	 * <crlf>     ::= CR LF
	 */

	token_begin = &line[0];
	
	/* handle the prefix! */
	/* TODO: split up into servername/nick!user@host */
	if (':' == token_begin[0]) {
		/* offset by 1 to ignore leading ':' */
		token_end = index(&token_begin[1], ' ');
		/* -1 to trim trailing character from index() */
		token_len = token_end - token_begin - 1;

		msg->prefix = strndup(&token_begin[1], token_len);

		/* move our offset, prefix increment to get off of space */
		token_begin = ++token_end;
	}

	/* 
	 * get our command, does not have a leading char -> no offset.
	 * params starts with a space, so we can use that.
	 * see above for how this works.
	 */
	token_end = index(&token_begin[0], ' ');
	token_len = token_end - token_begin;
	msg->command = strndup(&token_begin[0], token_len);
	token_begin = ++token_end;

	/* get our middle; we should be positioned at the beginning of params */
	if (':' != token_begin[0]) {
		token_end = index(&token_begin[0], ' ');
		token_len = token_end - token_begin;
		msg->middle = strndup(&token_begin[0], token_len);
		token_begin = ++token_end;
	}

	/* the rest should be our params trailing */
	token_end = index(&token_begin[1], '\r');
	token_len = token_end - token_begin - 1;
	msg->params = strndup(&token_begin[1], token_len);
	token_begin = ++token_end;


	return msg;
}
