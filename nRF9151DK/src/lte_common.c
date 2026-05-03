#include "lte_common.h"

#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(Tracker_LTE, LOG_LEVEL_INF);

K_SEM_DEFINE(lte_connected, 0, 1);
K_SEM_DEFINE(neighbors_found, 0, 1);

struct lte_geo_data geo_data;

void lte_handler(const struct lte_lc_evt *const evt)
{
    switch (evt->type) {
        case LTE_LC_EVT_NW_REG_STATUS:
            if (
                (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
                (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)
            ) {
                break;
            }

            k_sem_give(&lte_connected);
            dk_set_led_on(DK_LED2);

            break;

        case LTE_LC_EVT_RRC_UPDATE:
            LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
                    "Connected" : "Idle");
            break;

        /*case LTE_LC_EVT_PSM_UPDATE:
            LOG_INF("PSM parameter updated");
            if (evt->psm_cfg.active_time == -1) {
                LOG_ERR("Network rejected PSM parameter. Failed to enable PSM.");
            }
            break;

        case LTE_LC_EVT_EDRX_UPDATE:
            LOG_INF("eDRX parameter updated");
            break;*/

        case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
            LOG_INF("Neighbor cell measurement result received");
            
            geo_data.current_cell.mcc = evt->cells_info.current_cell.mcc;
            geo_data.current_cell.mnc = evt->cells_info.current_cell.mnc;
            geo_data.current_cell.id = evt->cells_info.current_cell.id;
            geo_data.current_cell.tac = evt->cells_info.current_cell.tac;
            geo_data.current_cell.rsrp = evt->cells_info.current_cell.rsrp;

            geo_data.ncells_count = MIN(evt->cells_info.ncells_count, MAX_NCELLS);
            for (uint8_t idx = 0; idx < geo_data.ncells_count; ++idx) {
                geo_data.ncells[idx].earfcn = evt->cells_info.neighbor_cells[idx].earfcn;
                geo_data.ncells[idx].pci = evt->cells_info.neighbor_cells[idx].phys_cell_id;
                geo_data.ncells[idx].rsrp = evt->cells_info.neighbor_cells[idx].rsrp;
            }

            geo_data.gci_cells_count = MIN(evt->cells_info.gci_cells_count, MAX_GCI_CELLS);
            for (uint8_t idx = 0; idx < geo_data.gci_cells_count; ++idx) {
                geo_data.gci_cells[idx].id = evt->cells_info.gci_cells[idx].id;
                geo_data.gci_cells[idx].mcc = evt->cells_info.gci_cells[idx].mcc;
                geo_data.gci_cells[idx].mnc = evt->cells_info.gci_cells[idx].mnc;
                geo_data.gci_cells[idx].tac = evt->cells_info.gci_cells[idx].tac;
                geo_data.gci_cells[idx].rsrp = evt->cells_info.gci_cells[idx].rsrp;
            }

            k_sem_give(&neighbors_found);
            break;

        default:
            break;
    }
}

int modem_configure()
{
    int err;
    
    LOG_INF("Initializing modem library");

    err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Failed to initialize the modem library, error %d", err);
        return err;
    }

    LOG_INF("Modem library initialized");
    /*LOG_INF("Requesting PSM");

    err = lte_lc_psm_req(false);
    if (err) {
        LOG_ERR("Failed to request PSM from modem, error %d", err);
    }

    LOG_INF("Requesting eDRX");
    
    err = lte_lc_edrx_req(false);
    if (err) {
        LOG_ERR("Failed to request eDRX from modem, error %d", err);
    }*/

    LOG_INF("Connecting to LTE network");

    err = lte_lc_connect_async(lte_handler);
    if (err) {
        LOG_ERR("Error when calling lte_lc_connect_async, error %d", err);
        return err;
    }

    return 0;
}
