#include "lte_common.h"
#include "gnss_common.h"
#include "coap_common.h"

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(Tracker_Main, LOG_LEVEL_INF);

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

    // Once DK connected to LTE network, setup CoAP
    err = coap_init();
    if (err) {
        LOG_ERR("CoAP configuration failed, error %d", err);
        return err;
    }

    LOG_INF("CoAP connection setup complete");

    // Once CoAP connection is established, start GNSS
    err = gnss_init_and_start();
    if (err) {
        LOG_ERR("GNSS could not start, error %d", err);
        return err;
    }

    LOG_INF("GNSS setup complete");

    while (1) {
        k_sem_take(&gps_fix_found, K_FOREVER);

        struct gnss_data gps_data = get_fix();

        err = coap_put(gps_data);
        if (err) {
            LOG_ERR("Failed to send CoAP data");
            continue;
        }

        LOG_INF("New GPS data sent");
    }

    return 0;
}
