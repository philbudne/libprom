#include "prom.h"

PROM_LABELED_COUNTER(pooh, "attr", "help for pooh");

PROM_SIMPLE_COUNTER_LABEL(pooh, honeypots);
PROM_SIMPLE_COUNTER_LABEL(pooh, friends);
PROM_SIMPLE_COUNTER_LABEL(pooh, trees);

void
main() {
    prom_format_vars(stdout);

    puts("---");
    PROM_SIMPLE_COUNTER_LABEL_INC(pooh, honeypots);
    prom_format_vars(stdout);

    puts("---");
    PROM_SIMPLE_COUNTER_LABEL_INC(pooh, trees);
    prom_format_vars(stdout);

    puts("---");
    /* Eeyore, Kanga, Roo, C.R., WOL, Piglet, Tigger, Rabbit, Gopher */
    PROM_SIMPLE_COUNTER_LABEL_INC_BY(pooh, friends, 9);
    prom_format_vars(stdout);
}

