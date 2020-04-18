libprom -- an exceedingly/excessively thin prometheus C client library

No external dependencies
    (you need to provide connection/thread-pool handling)

Written to be portable to POSIX/Un*x-like platforms:

* expects:
  * GNU-C __attribute__ (puts all variables in their own segment)
  * C11 _Atomic

* only requires standard tools (make & cc)

*NOT* a conforming client library!!!!

Only counters & gauges

All variables statically defined using macros

Three flavors of counter:
* PROM_SIMPLE_COUNTER(name,"help string")
  + integer values
  + implements:
    * PROM_SIMPLE_COUNTER_INC(name)
    * PROM_SIMPLE_COUNTER_INC_BY(name,val)
  + no labels (scalar)
* PROM_GETTER_COUNTER(name, "help string")
  + must define double valued getter function using PROM_GETTER_COUNTER_FN_PROTO(name)
  + no labels (scalar)
* PROM_FORMAT_COUNTER(name, "help string")
  + must define format function using PROM_FORMAT_COUNTER_FN_PROTO(name)
  + format function can output any number of lines w/ labels using PROM_PRINTF(f, ....)

Three flavors of gauge:
* PROM_SIMPLE_GAUGE(name,"help string")
  + integer values
  + implements:
    * PROM_SIMPLE_GAUGE_INC(name)
    * PROM_SIMPLE_GAUGE_INC_BY(name,val)
    * PROM_SIMPLE_GAUGE_SET(name,val)
  + no labels (scalar)
* PROM_GETTER_GAUGE(name, "help string")
  + must define double valued getter function using PROM_GETTER_GAUGE_FN_PROTO(name)
  + no labels (scalar)
* PROM_FORMAT_GAUGE(name, "help string")
  + must define format function using PROM_FORMAT_GAUGE_FN_PROTO(name)
  + format function can output any number of lines w/ labels using PROM_PRINTF(f, ....)
