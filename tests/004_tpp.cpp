// c++ -std=c++17 -o t t.cpp libprom.a

#include "prom.h"

PROM_SIMPLE_COUNTER(foo, "Foo");

int
main() {
	PROM_SIMPLE_COUNTER_INC(foo);
	prom_format_vars(stdout);
}
