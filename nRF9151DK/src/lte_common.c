#include "lte_common.h"

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(Tracker_LTE, LOG_LEVEL_INF);

K_SEM_DEFINE(lte_connected, 0, 1);

void lte_handler(const struct lte_lc_evt *const evt)
{
    switch (evt->type) {
        case LTE_LC_EVT_NW_REG_STATUS:
            if (
                (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
                (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)
            ) {
                break;
            }

            k_sem_give(&lte_connected);
            dk_set_led_on(DK_LED2);

            break;

        case LTE_LC_EVT_RRC_UPDATE:
            LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
                    "Connected" : "Idle");
            break;

        default:
            break;
    }
}

int modem_configure()
{
    int err;
    
    LOG_INF("Initializing modem library");

    err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Failed to initialize the modem library, error %d", err);
        return err;
    }

    LOG_INF("Modem library initialized");
    LOG_INF("Connecting to LTE network");

    err = lte_lc_connect_async(lte_handler);
    if (err) {
        LOG_ERR("Error when calling lte_lc_connect_async, error %d", err);
        return err;
    }

    return 0;
}
