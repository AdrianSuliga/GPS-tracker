#ifndef COAP_COMMON_H
#define COAP_COMMON_H

#include <stdio.h>
#include "gnss_common.h"

#define COAP_VERSION 1
#define COAP_MAX_MSG_LEN 1024

int coap_init();

int coap_put(struct gnss_data data);

#endif /* COAP_COMMON_H */
