#include "gnss_common.h"

#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(GPS_GNSS, LOG_LEVEL_INF);

void log_gnss_data(struct gnss_data *data)
{
    LOG_INF("(Long=%.6f, Lat=%.6f, Att=%.2f, Time=[%s])", 
            data->longitude,
            data->latitude,
            data->altitude,
            data->time_str);
}
