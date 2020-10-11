/*
 * net.h
 *
 * Shitbot is licensed under the GNU GPL, version 3 or later.
 * See the LICENSE.
 * Copyright (C) Kenneth B. Jensen 2021. All rights reserved.
 *
 */

#ifndef _NET_H_
#define _NET_H_

int tdprintf(int, const char *, ...);
int net_connect(char *, char *);
int net_close(int);

#endif /* _NET_H_ */
