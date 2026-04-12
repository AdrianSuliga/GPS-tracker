#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem.h>
#include <nrfx_ipc.h>
#include <pm_config.h>
#include <modem/lte_lc.h>

LOG_MODULE_REGISTER(L2E2, LOG_LEVEL_INF);

#define NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE 128

struct nrf_modem_lib_dfu_cb {
	void (*callback)(int dfu_res, void *ctx);
	void *context;
};

void dfu_handler(uint32_t dfu_res)
{
	LOG_DBG("Modem library update result %d", dfu_res);
	STRUCT_SECTION_FOREACH(nrf_modem_lib_dfu_cb, e) {
		LOG_DBG("Modem dfu callback: %p", e->callback);
		e->callback(dfu_res, e->context);
	}
}

void modem_handler(struct nrf_modem_fault_info *fault)
{
	LOG_ERR("Modem has crashed, reason 0x%x, PC: 0x%x",
		fault->reason, fault->program_counter);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	LOG_INF("Event: %d | NW Reg: %d", evt->type, evt->nw_reg_status);
    switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");

		break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle");
		break;
	default:
		break;
	}
}

static const struct nrf_modem_init_params init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.ctrl = {
		.base = PM_NRF_MODEM_LIB_CTRL_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE,
	},
	.shmem.tx = {
		.base = PM_NRF_MODEM_LIB_TX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE -
			NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE,
	},
	.shmem.rx = {
		.base = PM_NRF_MODEM_LIB_RX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE,
	},
	.fault_handler = modem_handler,
	.dfu_handler = dfu_handler,
};

static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library.");

    err = nrf_modem_init(&init_params);
    if (err) {
        LOG_ERR("Modem init failed with %d.", err);
        return err;
    }

    LOG_INF("Modem library initialized successfully.");
    LOG_INF("Trying to connect to LTE network.");

    err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
		return err;
	}

    LOG_INF("Request sent successfully.");

	return 0;
}

int main(void)
{
	int err;

	err = modem_configure();
	if (err) {
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

    while (1) {
        k_sleep(K_SECONDS(1));
    }
	
	return 0;
}
