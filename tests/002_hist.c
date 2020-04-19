#include "prom.h"

PROM_HISTOGRAM(my_histogram, "Histogram of observations");

const double limits[] = { 0.5, 1, 2.5, 5, 10, 20 };
PROM_HISTOGRAM_CUSTOM(my_custom, "Histogram of observations", limits);

int
main() {
    for (double v = 0.001; v <= 20.0; v *= 2) {
	printf(PROM_DOUBLE_FORMAT "\n", v);
	PROM_HISTOGRAM_OBSERVE(my_histogram, v);
	PROM_HISTOGRAM_OBSERVE(my_custom, v);
    }
    prom_format_vars(stdout);
    return 0;
}
