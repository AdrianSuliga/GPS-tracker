#ifndef LTE_COMMON_H
#define LTE_COMMON_H

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

extern struct k_sem lte_connected;

// Handler for LTE events
void lte_handler(const struct lte_lc_evt *const evt);

// Modem initialization and LTE connection
int modem_configure();

#endif /* LTE_COMMON_H */
