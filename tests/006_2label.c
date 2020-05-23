#include "prom.h"

PROM_2LABELED_COUNTER(things, "shape", "color", "Counts of things");
PROM_SIMPLE_COUNTER_2LABEL(things, circle, blue);
PROM_SIMPLE_COUNTER_2LABEL(things, square, green);
PROM_GETTER_COUNTER_2LABEL_FN(things, triangle, purple) {
    return 3;
}

int
main() {
    PROM_SIMPLE_COUNTER_2LABEL_INC(things, circle, blue);
    PROM_SIMPLE_COUNTER_2LABEL_INC_BY(things, square, green, 2);
    prom_format_vars(stdout);

    return 0;
}
