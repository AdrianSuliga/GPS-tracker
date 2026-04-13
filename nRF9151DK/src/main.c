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

    // Once CoAP connection is prepared, send sample GNSS data
    struct gnss_data sample = {
        .longitude = 10.0f,
        .latitude = 12.0f,
        .altitude = 101.1f,
        .time_str = "12:00:00.000"
    };

    LOG_INF("Sending GNSS sample to %s", CONFIG_COAP_SERVER_HOSTNAME);

    err = coap_put(&sample, sizeof(struct gnss_data));
    if (err < 0) {
        LOG_ERR("CoAP PUT failed, error %d", err);
        return err;
    }

    LOG_INF("Message sent successfully");

    return 0;
}
