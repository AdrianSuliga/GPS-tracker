#include "lte_common.h"

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(GPS_Main, LOG_LEVEL_INF);

int main()
{
    int err;

    err = dk_leds_init();
    if (err != 0) {
        LOG_ERR("Failed to initialize the DK library, error %d", err);
        return err;
    }

    // Initialize modem and connect to LTE network
    err = modem_configure();
    if (err) {
        LOG_ERR("Failed to configure modem");
        return err;
    }

    // Wait for LTE connection
    k_sem_take(&lte_connected, K_FOREVER);

    LOG_INF("Connected to LTE network");

    return 0;
}
