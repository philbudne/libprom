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
