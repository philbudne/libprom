#include "prom.h"

PROM_LABELED_COUNTER(pooh, "attr", "help for pooh");

PROM_SIMPLE_COUNTER_LABEL(pooh, honeypots);
PROM_SIMPLE_COUNTER_LABEL(pooh, friends);
PROM_SIMPLE_COUNTER_LABEL(pooh, trees);

int
main() {
    PROM_SIMPLE_COUNTER_LABEL_INC(pooh, honeypots);
    PROM_SIMPLE_COUNTER_LABEL_INC(pooh, trees);
    /* Eeyore, Kanga, Roo, C.R., WOL, Piglet, Tigger, Rabbit, Gopher */
    PROM_SIMPLE_COUNTER_LABEL_INC_BY(pooh, friends, 9);
    prom_format_vars(stdout);

    return 0;
}
