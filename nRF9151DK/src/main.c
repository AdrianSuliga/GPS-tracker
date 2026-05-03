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
    /*err = gnss_init_and_start();
    if (err) {
        LOG_ERR("GNSS could not start, error %d", err);
        return err;
    }

    LOG_INF("GNSS setup complete");*/

    struct lte_lc_ncellmeas_params params = {
        .search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT,
        .gci_count = 15
    };

    while (1) {
        /*k_sem_take(&gps_fix_found, K_FOREVER);

        struct gnss_data gps_data = get_fix();

        err = coap_put(gps_data);
        if (err) {
            LOG_ERR("Failed to send CoAP data");
            continue;
        }

        LOG_INF("New GPS data sent");*/

        /*err = coap_ping();
        if (err) {
            LOG_ERR("Failed to sent CoAP data");
            continue;
        }*/

        err = lte_lc_neighbor_cell_measurement(&params);
        if (err < 0) {
            LOG_ERR("Neighbor cell measurement failed, error %d", err);
            continue;
        }

        err = coap_put_lte(&geo_data);
        if (err) {
            LOG_ERR("Failed to send CoAP data");
        }

        k_sem_take(&neighbors_found, K_FOREVER);
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
