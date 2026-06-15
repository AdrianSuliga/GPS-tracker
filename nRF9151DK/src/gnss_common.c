#include "gnss_common.h"

#include <stdio.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Tracker_GNSS, LOG_LEVEL_INF);

K_SEM_DEFINE(gps_fix_found, 0, 1);

struct gnss_data gps_data = {
    .longitude = 0.0f,
    .latitude = 0.0f,
    .altitude = 0.0f,
    .time = { .year = 1, .month = 1, .day = 1,
              .hour = 0, .minute = 0, .second = 0 }
};

static struct nrf_modem_gnss_pvt_data_frame pvt_data;

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

                gps_data.longitude = pvt_data.longitude;
                gps_data.latitude = pvt_data.latitude;
                gps_data.altitude = pvt_data.altitude;
                gps_data.time.year = pvt_data.datetime.year;
                gps_data.time.month = pvt_data.datetime.month;
                gps_data.time.day = pvt_data.datetime.day;
                gps_data.time.hour = pvt_data.datetime.hour;
                gps_data.time.minute = pvt_data.datetime.minute;
                gps_data.time.second = pvt_data.datetime.seconds;

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
            break;
    }
}

int gnss_init_and_start()
{
    int err;

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

    err = nrf_modem_gnss_prio_mode_enable();
    if (err != 0) {
        LOG_ERR("Failed to enable GNSS priority mode, error %d", err);
        return err;
    }

    return 0;
}
