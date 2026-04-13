#include "gnss_common.h"

#include <stdio.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Tracker_GNSS, LOG_LEVEL_INF);

K_SEM_DEFINE(gps_fix_found, 0, 1);

static struct nrf_modem_gnss_pvt_data_frame pvt_data;

void log_gnss_data(struct gnss_data data)
{
    LOG_INF("(Long=%.6f, Lat=%.6f, Att=%.2f, Time=[%s])", 
            data.longitude,
            data.latitude,
            data.altitude,
            data.time_str);
}

struct gnss_data get_fix()
{
    struct gnss_data result = {
        .longitude = pvt_data.longitude,
        .latitude = pvt_data.latitude,
        .altitude = pvt_data.altitude,
        .time_str = ""
    };

    return result;
}

static void gnss_event_handler(int event)
{
    int err, num_satelites;

    switch (event) {
        case NRF_MODEM_GNSS_EVT_PVT:
            num_satelites = 0;
            for (int i = 0; i < 12; ++i) {
                if (pvt_data.sv[i].signal != 0) {
                    ++num_satelites;
                }
            }

            LOG_INF("Searching. Current satelites: %d", num_satelites);
            err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
            if (err) {
                LOG_ERR("Failed to read nRF GNSS, error %d", err);
                return;
            }

            if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
                LOG_INF("GNSS got fix");

                k_sem_give(&gps_fix_found);
            }

            if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
                LOG_INF("GNSS blocked by LTE activity");
            }

            if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
                LOG_INF("Insufficient GNSS time windows");
            }
            break;

        case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
            LOG_INF("GNSS has woken up");
            break;

        case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
            LOG_INF("GNSS is going to sleep");
            break;

        default:
            LOG_INF("Unknown event %d", event);
            break;
    }
}

int gnss_init_and_start()
{
    int err;

    err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
    if (err != 0) {
        LOG_ERR("Failed to activate GNSS functional mode, error %d", err);
        return err;
    }

    err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
    if (err != 0) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return err;
	}

    err = nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_PERIODIC_INTERVAL);
	if (err != 0) {
		LOG_ERR("Failed to set GNSS fix interval, error %d", err);
		return err;
	}

    err = nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_PERIODIC_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Failed to set GNSS fix retry, error %d", err);
		return err;
	}

	LOG_INF("Starting GNSS");

    err = nrf_modem_gnss_start();
	if (err != 0) {
		LOG_ERR("Failed to start GNSS, error %d", err);
		return err;
	}

    return 0;
}
