#include "coap_common.h"
#include "gnss_common.h"

#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>

LOG_MODULE_REGISTER(Tracker_CoAP, LOG_LEVEL_INF);

static struct sockaddr_storage server;
static uint8_t coap_buffer[COAP_MAX_MSG_LEN];
static uint16_t next_token;
static int sock;

static int server_resolve()
{
    int err;

    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM
    };
    char ipv4_addr[NET_IPV4_ADDR_LEN];

    err = getaddrinfo(CONFIG_COAP_SERVER_HOSTNAME, NULL, &hints, &result);
    if (err != 0) {
        LOG_ERR("getaddrinfo failed, error %d", err);
        return -EIO;
    }

    if (result == NULL) {
        LOG_ERR("Address for %s not found", CONFIG_COAP_SERVER_HOSTNAME);
        return -ENOENT;
    }

    struct sockaddr_in *server4 = ((struct sockaddr_in*)&server);

    server4->sin_addr.s_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr;
    server4->sin_family = AF_INET;
    server4->sin_port = htons(CONFIG_COAP_SERVER_PORT);

    inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr, sizeof(ipv4_addr));
    LOG_INF("%s address resolved as %s", CONFIG_COAP_SERVER_HOSTNAME, ipv4_addr);

    freeaddrinfo(result);

    return 0;
}

static int client_init()
{
    int err;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);
    if (sock < 0) {
        LOG_ERR("Failed to create CoAP socket, error %d", sock);
        return sock;
    }

    enum {
        NONE = 0,
        OPTIONAL = 1,
        REQUIRED = 2
    };

    int verify = REQUIRED;

    err = setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
    if (err) {
	    LOG_ERR("Failed to setup peer verification, errno %d\n", errno);
	    return -errno;
    }

    err = setsockopt(sock, SOL_TLS, TLS_HOSTNAME, CONFIG_COAP_SERVER_HOSTNAME, strlen(CONFIG_COAP_SERVER_HOSTNAME));
    if (err) {
        LOG_ERR("Failed to setup TLS hostname (%s), errno %d\n", CONFIG_COAP_SERVER_HOSTNAME, errno);
	    return -errno;
    }

    sec_tag_t sec_tag_list[] = { SEC_TAG };
    err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
    if (err) {
        LOG_ERR("Failed to setup socket security tag, errno %d\n", errno);
	    return -errno;
    }

    err = connect(sock, (const struct sockaddr*)&server, sizeof(struct sockaddr_in));
    if (err < 0) {
        LOG_ERR("Failed to connect to CoAP socket, error %d", err);
        return err;
    }

    LOG_INF("Successfully connected to %s", CONFIG_COAP_SERVER_HOSTNAME);

    next_token = sys_rand32_get();

    return 0;
}

static int coap_put(char *data, uint16_t size, char *resource)
{
    int err;
    struct coap_packet coap_request;

    next_token = sys_rand32_get();

    err = coap_packet_init(&coap_request, coap_buffer, sizeof(coap_buffer),
                        COAP_VERSION, COAP_TYPE_NON_CON, sizeof(next_token),
                        (uint8_t*)&next_token, COAP_METHOD_PUT, coap_next_id());
    if (err < 0) {
        LOG_ERR("Failed to create CoAP PUT request, error %d", err);
        return err;
    }
    
    err = coap_packet_append_option(&coap_request, COAP_OPTION_URI_PATH,
                                    (uint8_t*)resource,
                                    strlen(resource));
    if (err < 0) {
        LOG_ERR("Failed to append CoAP option, error %d", err);
        return err;
    }

    const uint8_t text = COAP_CONTENT_FORMAT_APP_JSON;
    err = coap_packet_append_option(&coap_request, COAP_OPTION_CONTENT_FORMAT,
                                    &text, sizeof(text));
    if (err < 0) {
        LOG_ERR("Failed to append CoAP option, error %d", err);
        return err;
    }

    err = coap_packet_append_payload_marker(&coap_request);
    if (err < 0) {
        LOG_ERR("Failed to append CoAP payload marker, error %d", err);
        return err;
    }

    err = coap_packet_append_payload(&coap_request, (uint8_t*)data, size);
    if (err < 0) {
        LOG_ERR("Failed to append payload to CoAP packet, error %d", err);
        return err;
    }

    err = send(sock, coap_request.data, coap_request.offset, 0);
    if (err < 0) {
        LOG_ERR("Failed to send CoAP request, error %d", err);
        return err;
    }

    LOG_INF("CoAP PUT request sent");

    return 0;
}

int coap_init()
{
    int err;

    err = server_resolve();
    if (err != 0) {
        return err;
    }

    err = client_init();
    if (err != 0) {
        return err;
    }

    return 0;
}

int coap_put_gnss(struct gnss_data *data)
{
    char msg[256];
    uint16_t n = snprintf(msg, sizeof(msg), "{gnss: {long:%.6f,lat:%.6f,alt:%.2f,time:%u-%u-%u %u:%u:%u}}",
                            data->longitude, data->latitude, data->altitude,
                            data->time.year, data->time.month, data->time.day,
                            data->time.hour, data->time.minute, data->time.second);

    return coap_put(msg, n, CONFIG_COAP_GNSS_RESOURCE);
}

int coap_put_lte(struct lte_geo_data *data)
{
    char msg[512];
    int offset = 0;

    offset += snprintf(msg + offset, sizeof(msg) - offset,
        "{geo: {"
        "current_cell: {"
            "mcc:%d,"
            "mnc:%d,"
            "id:%u,"
            "tac:%u,"
            "rsrp:%d"
        "},",
        data->current_cell.mcc,
        data->current_cell.mnc,
        data->current_cell.id,
        data->current_cell.tac,
        data->current_cell.rsrp
    );

    offset += snprintf(msg + offset, sizeof(msg) - offset,
        " gci_cells: ["
    );

    for (int i = 0; i < data->gci_cells_count; i++) {
        offset += snprintf(msg + offset, sizeof(msg) - offset,
            "{"
                "mcc:%d,"
                "mnc:%d,"
                "id:%u,"
                "tac:%u,"
                "rsrp:%d"
            "}%s",
            data->gci_cells[i].mcc,
            data->gci_cells[i].mnc,
            data->gci_cells[i].id,
            data->gci_cells[i].tac,
            data->gci_cells[i].rsrp,
            (i < data->gci_cells_count - 1) ? "," : ""
        );
    }

    offset += snprintf(msg + offset, sizeof(msg) - offset, "]"
        "}}"
    );

    return coap_put(msg, offset, CONFIG_COAP_GEO_RESOURCE);
}
