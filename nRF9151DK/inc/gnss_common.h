#ifndef GNSS_COMMON_H
#define GNSS_COMMON_H

struct gnss_data {
    double longitude;
    double latitude;
    double altitude;
    char time_str[13];
};

void log_gnss_data(struct gnss_data *data);

#endif /* GNSS_COMMON_H */
