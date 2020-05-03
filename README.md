libprom -- an exceedingly/excessively thin prometheus C client library

No external dependencies (just cc and make)

Written to be portable to POSIX/Un*x-like platforms:

Created to embed in legacy applications

* Only requires standard tools (make & gcc/clang)
  * C99 64-bit long long (available since gcc 2.95?)
  * atomic add (available since gcc 4.1?)
  * GNU-C attribute (puts all variables in their own loader section)

* Tested on:
  * Ubuntu 20.04 (gcc 9.3)
  * FreeBSD 12 (clang 8.0.1)
  * macOS Catalina (10.14)

*NOT* a conforming client library!!!!

* Implements counters, gauges and histogram

All variables statically defined using macros

Three flavors of counter:
* PROM_SIMPLE_COUNTER(name,"help string")
  + 64-bit integer values
  + implements:
    * PROM_SIMPLE_COUNTER_INC(name)
    * PROM_SIMPLE_COUNTER_INC_BY(name,val)
  + Increments are atomic (thread safe)
  + no labels (scalar)
* PROM_GETTER_COUNTER(name, "help string")
  + must define double valued getter function via:
    * PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_bytes) { ... }
  + no labels (scalar)
* PROM_FORMAT_COUNTER(name, "help string")
  + must define format function via PROM_FORMAT_COUNTER_FN_PROTO(name) { ... }
  + format function can output any number of lines w/ labels
      + start output of a value with `prom_format_start(f, &state, pvp);`
      + output any number of labels: `prom_format_label(f, &state, "lbl", "%d", val);`
      + output value with one of:
	* prom_format_value_pv(f, &state, prom_value_var);
	* prom_format_value_dbl(f, &state, double_var);
	* int prom_format_value(f, &state, "%d", value);

Three flavors of gauge:
* PROM_SIMPLE_GAUGE(name,"help string")
  + 64-bit integer values
  + atomic increment/decrement
  + implements:
    * PROM_SIMPLE_GAUGE_INC(name)
    * PROM_SIMPLE_GAUGE_DEC(name)
    * PROM_SIMPLE_GAUGE_INC_BY(name,val)
    * PROM_SIMPLE_GAUGE_SET(name,val)
  + no labels (scalar)
* PROM_GETTER_GAUGE(name, "help string")
  + must define double valued getter function via PROM_GETTER_GAUGE_FN_PROTO(name)
  + no labels (scalar)
* PROM_FORMAT_GAUGE(name, "help string")
  + must define format function using PROM_FORMAT_GAUGE_FN_PROTO(name)
  + format function can output any number of lines w/ labels (see counters)

Two flavors of histogram:
* PROM_HISTOGRAM(name,"help string")
  + default limits: 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10
  + PROM_HISTOGRAM_OBSERVE(name, value)
* PROM_HISTOGRAM_CUSTOM(name, "help string", array_of_double_limits)
  + PROM_HISTOGRAM_OBSERVE(name, value)

Request processing:
* s = prom_listen(int port, int proto, int nonblock);
* prom_pool_init(int threads, const char *exporter_name);
* prom_accept(s);

Options:
* omit prom_pool_init call
* supply prom_dispatch(socket);
* call prom_http_request(FILE *in, FILE *out, const char *exporter_name);
