// implement sub-fractional HTTP for libprom
// Phil Budne 2020-04-11

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

#include <string.h>
#include <unistd.h>			/* write */

#include "prom.h"

PROM_LABELED_COUNTER(promhttp_metric_handler_requests_total, "code",
		  "Total number of scrapes by HTTP status code");

PROM_SIMPLE_COUNTER_LABEL(promhttp_metric_handler_requests_total,200); // returned page
PROM_SIMPLE_COUNTER_LABEL(promhttp_metric_handler_requests_total,400); // bad request
PROM_SIMPLE_COUNTER_LABEL(promhttp_metric_handler_requests_total,500); // internal error
PROM_SIMPLE_COUNTER_LABEL(promhttp_metric_handler_requests_total,503); // service unavail

// dodge to avoid complaints about unused result
// (until the compilers get smarter)
static int
send_str(int fd, const char *str) {
    return write(fd, str, strlen(str));
}

void
prom_http_interr(int fd)
{
    send_str(fd, "HTTP/1.0 500 Internal Server Error\r\n");
    PROM_SIMPLE_COUNTER_LABEL_INC(promhttp_metric_handler_requests_total,500);
}

void
prom_http_unavail(int fd)
{
    send_str(fd, "HTTP/1.0 503 Service Unavailable\r\n");
    PROM_SIMPLE_COUNTER_LABEL_INC(promhttp_metric_handler_requests_total,503);
}
#undef SEND

int
prom_http_request(PROM_FILE *in, PROM_FILE *out, const char *who) {
    char line[128];
    char cmd[128];
    char path[128];
    char proto[128];

    if (!PROM_GETS(line, sizeof(line), in)) {
	// XXX count??
	return -1;
    }

    
    switch (sscanf(line, "%s %s %s", cmd, path, proto)) {
    case 2:
	proto[0] = '\0';
	/* FALLTHRU */
    case 3:
	if (strcasecmp(cmd, "get") == 0)
	    break;
	/* FALLTHRU */
    default:
	// give an HTTP 1.0 response regardless; trying to keep it 99%
	PROM_PRINTF(out, "HTTP/1.0 400 Bad Request\r\n\r\n");
	PROM_SIMPLE_COUNTER_LABEL_INC(promhttp_metric_handler_requests_total,400);
	return 0; // for testing: not an I/O error!
    }

    if (proto[0]) {	    // eat headers if HTTP/1.0-like request
	while (PROM_GETS(line, sizeof(line), in) &&
	       line[0] != '\r' && line[0] != '\n')
	    ;
	PROM_PRINTF(out, "HTTP/1.0 200 OK\r\n"
		    "Server: %s exporter (libprom)\r\n", who);
	// XXX need Date: ?? I hope not!!!
    }

    PROM_SIMPLE_COUNTER_LABEL_INC(promhttp_metric_handler_requests_total,200);
    if (strcmp(path, "/metrics") == 0) {
	if (proto[0])
	    PROM_PRINTF(out, "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n\r\n");

	// XXX if Content-Length: becomess necessary,
	// handle by providing/requiring a prom_fmemopen function??
	prom_format_vars(out);
    }
    else {
	if (proto[0])
	    PROM_PRINTF(out, "Content-Type: text/html; charset=utf-8\r\n\r\n");
	PROM_PRINTF(out,
		    "<html>\r\n"
		    "<head><title>%s exporter</title></head>\r\n"
		    "<body>\r\n"
		    "<h1>%s exporter</h1>\r\n"
		    "<a href=\"/metrics\">Metrics</a></body>\r\n"
		    "</html>\r\n", who, who);
    }
    // NOTE!! we don't send Content-Length: so connection MUST be closed!
    // "Connection: close" is implied by HTTP/1.0 response
    return 0;
}

