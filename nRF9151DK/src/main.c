#include "lte_common.h"
#include "gnss_common.h"

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

    // Once DK connected to LTE network, send sample
    // GPS data to COAP server
    struct gnss_data sample = {
        .longitude = 10.0f,
        .latitude = 12.0f,
        .altitude = 101.1f,
        .time_str = "12:00:00.000"
    };

    LOG_INF("Sending GNSS sample to COAP server:");
    log_gnss_data(&sample);

    return 0;
}
