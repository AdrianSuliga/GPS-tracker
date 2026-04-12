#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(GPS_Tracker, LOG_LEVEL_INF);

static K_SEM_DEFINE(lte_connected, 0, 1);

static void lte_handler(const struct lte_lc_evt *const evt)
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

static int modem_configure()
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

int main()
{
    int err;

    err = dk_leds_init();
    if (err != 0) {
        LOG_ERR("Failed to initialize the DK library, error %d", err);
        return err;
    }

    err = modem_configure();
    if (err) {
        LOG_ERR("Failed to configure modem");
        return err;
    }

    k_sem_take(&lte_connected, K_FOREVER);

    LOG_INF("Connected to LTE network");

    return 0;
}
