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

    // Initialize modem
    err = modem_configure();
    if (err) {
        LOG_ERR("Failed to configure modem");
        return err;
    }

    // Once DK connected to LTE network, start GNSS
    err = gnss_init_and_start();
    if (err) {
        LOG_ERR("GNSS could not start, error %d", err);
        return err;
    }

    LOG_INF("GNSS setup complete");

    struct lte_lc_ncellmeas_params params = {
        .search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE,
        .gci_count = MAX_GCI_CELLS
    };

    while (true) {
        // Wait for GPS data
        LOG_INF("Waiting for GPS data");
        err = k_sem_take(&gps_fix_found, K_SECONDS(10));

        // Establish LTE connection to send data
        LOG_INF("Waiting for LTE connection");
        err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
        if (err) {
            LOG_ERR("Failed to establish LTE connection");
            continue;
        }

        k_sem_take(&lte_connected, K_FOREVER);
        LOG_INF("LTE connection established");

        // Setup CoAP
        err = coap_init();
        if (err) {
            LOG_ERR("Failed to setup CoAP connection");
            continue;
        }

        // Send GPS data to CoAP server
        err = coap_put_gnss(&gps_data);
        if (err) {
            LOG_ERR("Failed to send CoAP data");
        } else {
            LOG_INF("New GPS data sent");
        }

        // Request neighbor cell measurement
        err = lte_lc_neighbor_cell_measurement(&params);
        if (err < 0) {
            LOG_ERR("Neighbor cell measurement failed, error %d", err);
            continue;
        }

        // Wait for neighbor cell measurement results
        LOG_INF("Waiting for cell measurement data");
        k_sem_take(&neighbors_found, K_FOREVER);

        // Send neighbor cell data to CoAP server
        err = coap_put_lte(&geo_data);
        if (err) {
            LOG_ERR("Failed to send CoAP data");
        } else {
            LOG_INF("New cell measurement data sent");
        }

        // Break LTE connection so GPS has time for getting fix
        err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);
        if (err) {
            LOG_ERR("Failed to deactivate LTE");
            continue;
        }
    }

    return 0;
}
