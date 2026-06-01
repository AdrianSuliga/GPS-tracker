#ifndef COAP_COMMON_H
#define COAP_COMMON_H

#include <stdio.h>
#include "gnss_common.h"
#include "lte_common.h"

#define COAP_VERSION 1
#define COAP_MAX_MSG_LEN 1024

#define SEC_TAG 12

int coap_init();

int coap_put_gnss(struct gnss_data *data);

int coap_put_lte(struct lte_geo_data *data);

#endif /* COAP_COMMON_H */
