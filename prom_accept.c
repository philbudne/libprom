// accept server connections
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

#include <string.h>

#include "prom.h"

// w/ blocking socket never returns (run in a thread)
// w/ non-blocking socket, accepts/dispatches & returns

void
prom_accept(int s) {
    for (;;) {
	int conn;
	struct sockaddr sa;
	socklen_t addrlen = sizeof(sa);

	memset(&sa, 0, addrlen);
	conn = accept(s, &sa, &addrlen);
	if (conn < 0)
	    break;
	// NOTE! new socket is O_NONBLOCK if listener was!
	prom_dispatch(conn);	// XXX pass sa?!
    }
}
