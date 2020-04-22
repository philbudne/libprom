// set up TCP socket listener
// Phil Budne 2020-04-21

/*-
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2020, Philip L. Budne
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <string.h>			/* memset */
#include <unistd.h>			/* close */
#include <stdlib.h>			/* free */
#include <netdb.h>			/* getaddrinfo */

#include "prom.h"

int
prom_listen(int port, int family, int nonblock) {
    struct addrinfo hints, *res, *tr;
    char pstr[10];
    int s, af;

    switch (family) {
    case 4: af = AF_INET; break;
    case 6: af = AF_INET6; break;
    default: af = AF_UNSPEC; break;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(pstr, sizeof(pstr), "%d", port);
    if (getaddrinfo(NULL, pstr, &hints, &res) != 0)
	return -1;

    s = -1;
    for (tr = res; tr; tr = tr->ai_next) {
	s = socket(tr->ai_family, tr->ai_socktype, tr->ai_protocol);
	if (s >= 0) {
	    int on = 1;
	    // allow quick restart
	    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	    if (bind(s, tr->ai_addr, tr->ai_addrlen) == 0)
		break;
	    close(s);
	    s = -1;
	}
    }
    free(res);

    if (s < 0)
	return -1;

    if (nonblock)
	fcntl(s, F_SETFD, O_NONBLOCK);

    listen(s, 5);

    return s;
}
