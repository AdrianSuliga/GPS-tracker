#ifndef GNSS_COMMON_H
#define GNSS_COMMON_H

#include <zephyr/kernel.h>

extern struct k_sem gps_fix_found;

// XX:XX:XX.XXX\0
#define UTC_TIME_FORMAT_LEN 13

struct gnss_data {
    double longitude;
    double latitude;
    double altitude;
    char time_str[UTC_TIME_FORMAT_LEN];
};

void log_gnss_data(struct gnss_data data);

struct gnss_data get_fix();

int gnss_init_and_start();

#endif /* GNSS_COMMON_H */
