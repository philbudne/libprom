#include "prom.h"

PROM_LABELED_COUNTER(pooh, "attr", "help for pooh");

PROM_SIMPLE_COUNTER_LABEL(pooh, honeypots);
PROM_SIMPLE_COUNTER_LABEL(pooh, friends);
PROM_GETTER_COUNTER_LABEL_FN(pooh, trees) {
    return 1;
}

PROM_LABELED_GAUGE(sky, "attr", "help for sky");
PROM_SIMPLE_GAUGE_LABEL(sky, sun);
PROM_GETTER_GAUGE_LABEL_FN(sky, clouds) {
    return 1e6 + 1;
}

int
main() {
    PROM_SIMPLE_COUNTER_LABEL_INC(pooh, honeypots);
    /* Eeyore, Kanga, Roo, C.R., WOL, Piglet, Tigger, Rabbit, Gopher */
    PROM_SIMPLE_COUNTER_LABEL_INC_BY(pooh, friends, 9);

    PROM_SIMPLE_GAUGE_LABEL_SET(sky, sun, 1);

    prom_format_vars(stdout);

    return 0;
}

