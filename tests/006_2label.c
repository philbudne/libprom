#include "prom.h"

PROM_2LABELED_COUNTER(things, "shape", "color", "Counts of things");
PROM_SIMPLE_COUNTER_2LABEL(things, circle, blue);
PROM_SIMPLE_COUNTER_2LABEL(things, square, green);
PROM_GETTER_COUNTER_2LABEL_FN(things, triangle, purple) {
    return 3;
}

PROM_2LABELED_GAUGE(bananas, "type", "ripe", "Numbers of bananas on hand");
PROM_SIMPLE_GAUGE_2LABEL(bananas, cavendish, yes);
PROM_SIMPLE_GAUGE_2LABEL(bananas, cavendish, no);
PROM_GETTER_GAUGE_2LABEL_FN(bananas, lady_finger, yes) {
    return 3;
}

int
main() {
    PROM_SIMPLE_COUNTER_2LABEL_INC(things, circle, blue);
    PROM_SIMPLE_COUNTER_2LABEL_INC_BY(things, square, green, 2);

    PROM_SIMPLE_GAUGE_2LABEL_INC_BY(bananas, cavendish, no, 3);

    // one ripened
    PROM_SIMPLE_GAUGE_2LABEL_DEC(bananas, cavendish, no);
    PROM_SIMPLE_GAUGE_2LABEL_INC(bananas, cavendish, yes);

    prom_format_vars(stdout);

    return 0;
}
