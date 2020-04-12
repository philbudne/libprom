#include "prom.h"

PROM_SIMPLE_COUNTER(foo, "help for foo");

void out() {
    _PROM_SIMPLE_COUNTER_NAME(foo).format(stdout, &_PROM_SIMPLE_COUNTER_NAME(foo));
}

void
main() {
    out();
    PROM_SIMPLE_COUNTER_INC(foo);
    out();
    PROM_SIMPLE_COUNTER_INC_BY(foo, 2);
    out();
}

