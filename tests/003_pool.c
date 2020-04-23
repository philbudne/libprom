#include "prom.h"

int
main() {
    int s = prom_listen(8888, 0, 0);
    prom_process_init();		/* force load */
    prom_pool_init(3, "tpool");
    prom_accept(s);
}
