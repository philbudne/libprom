libprom -- an exceedingly/excessively thin prometheus C client library

No external dependencies

Written to be portable to POSIX/Un*x-like platforms:

Created to embed in legacy applications, without Docker and cmake

* Only requires standard tools (make & gcc/clang)
  * C99 long long (available since gcc 2.95?)
  * atomic add (available since gcc 4.1?)
  * GNU-C attribute (puts all variables in their own loader section)

* Tested on:
  * Ubuntu 20.04
  * FreeBSD 12
  * macOS Catalina (10.14)

*NOT* a conforming client library!!!!

* Implements counters, gauges and histogram

All variables statically defined using macros

Three flavors of counter:
* PROM_SIMPLE_COUNTER(name,"help string")
  + integer values
  + implements:
    * PROM_SIMPLE_COUNTER_INC(name)
    * PROM_SIMPLE_COUNTER_INC_BY(name,val)
  + no labels (scalar)
* PROM_GETTER_COUNTER(name, "help string")
  + must define double valued getter function
  + no labels (scalar)
* PROM_FORMAT_COUNTER(name, "help string")
  + must define format function using PROM_FORMAT_COUNTER_FN_PROTO(name)
  + format function can output any number of lines w/ labels

Three flavors of gauge:
* PROM_SIMPLE_GAUGE(name,"help string")
  + integer values
  + implements:
    * PROM_SIMPLE_GAUGE_INC(name)
    * PROM_SIMPLE_GAUGE_DEC(name)
    * PROM_SIMPLE_GAUGE_INC_BY(name,val)
    * PROM_SIMPLE_GAUGE_SET(name,val)
  + no labels (scalar)
* PROM_GETTER_GAUGE(name, "help string")
  + must define double valued getter function
  + no labels (scalar)
* PROM_FORMAT_GAUGE(name, "help string")
  + must define format function using PROM_FORMAT_GAUGE_FN_PROTO(name)
  + format function can output any number of lines w/ labels

Two flavors of histogram:
* PROM_HISTOGRAM(name,"help string")
  + default limits: 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10
  + PROM_HISTOGRAM_OBSERVE(name, value)
* PROM_HISTOGRAM_CUSTOM(name, "help string", array_of_double_limits)
  + PROM_HISTOGRAM_OBSERVE(name, value)
