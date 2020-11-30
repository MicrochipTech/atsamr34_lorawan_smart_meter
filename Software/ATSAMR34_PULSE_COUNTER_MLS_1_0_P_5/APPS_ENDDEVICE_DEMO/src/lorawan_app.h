#ifndef LORAWAN_APP_H
#define	LORAWAN_APP_H

#include "lorawan.h"
#include "system_task_manager.h"
#include "pmm.h"

/*** GLOBAL FUNCTION PROTOTYPES ************************************************/
void lorawan_app_init(void) ;
void lorawan_app_configuration(IsmBand_t bandSelection) ;
bool lorawan_app_join(void) ;
void lorawan_app_transmit(TransmissionType_t type, uint8_t fport, void* data, uint8_t len) ;
void lorawan_app_run_tasks(void) ;
PMM_Status_t lorawan_app_sleep(void) ;
SYSTEM_TaskStatus_t APP_TaskHandler(void) ;
void lorawan_app_wakeup(void) ;
void lorawan_app_setDevEui(const uint8_t *devEui, uint8_t devEuiLen) ;
void lorawan_app_setAppEui(const uint8_t *appEui, uint8_t appEuiLen) ;
void lorawan_app_setAppKey(const uint8_t *appKey, uint8_t appKeyLen) ;


#endif