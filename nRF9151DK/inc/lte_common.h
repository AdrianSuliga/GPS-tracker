#ifndef LTE_COMMON_H
#define LTE_COMMON_H

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>

#define MAX_GCI_CELLS 15

extern struct k_sem lte_connected;
extern struct k_sem neighbors_found;

struct cell_data {
    int mcc;
    int mnc;
    uint32_t id;
    uint32_t tac;
    int16_t rsrp;
};

struct lte_geo_data {
    struct cell_data current_cell;

    uint8_t gci_cells_count;
    struct cell_data gci_cells[MAX_GCI_CELLS];
};

extern struct lte_geo_data geo_data;

// Handler for LTE events
void lte_handler(const struct lte_lc_evt *const evt);

// Modem initialization and LTE connection
int modem_configure();

#endif /* LTE_COMMON_H */
