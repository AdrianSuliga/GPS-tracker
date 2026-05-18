#ifndef GNSS_COMMON_H
#define GNSS_COMMON_H

#include <zephyr/kernel.h>

extern struct k_sem gps_fix_found;

struct fix_time {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct gnss_data {
    double longitude;
    double latitude;
    double altitude;
    struct fix_time time;
};

extern struct gnss_data gps_data;

// Initialize and start GNSS
int gnss_init_and_start();

#endif /* GNSS_COMMON_H */
