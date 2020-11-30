/*
* Test application
*/

#include <asf.h>
#include "radio_driver_hal.h"
#include "sw_timer.h"
#include "aes_engine.h"
#include "sio2host.h"
#include "delay.h"
#include "sleep_timer.h"
#include "pds_interface.h"
#include "pmm.h"
#include "conf_pmm.h"
#include "lorawan.h"

//#define UPLINK_ENABLED

uint8_t app_eui[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t dev_eui[8] = {0x00, 0x04, 0x25, 0x19, 0x18, 0x01, 0xd4, 0xa7};
uint8_t app_key[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t timer;
bool joined = false;
bool timer_fired = false;

LorawanSendReq_t req;

void app_init(void);
void join_response(StackRetStatus_t status);
void data_received(void *handle, appCbParams_t *params);
void timer_callback(void);
SYSTEM_TaskStatus_t APP_TaskHandler(void);
void resources_init(void);
void resources_deinit(void);
PMM_Status_t sleep_start(void);
void sleep_ended(uint32_t slept_duration);

int main(void) {
	app_init();
	while(1) {
		SYSTEM_RunTasks();
		sleep_start();
	}	
}

void app_init(void) {
	system_init();
	board_init();
	delay_init();
	INTERRUPT_GlobalInterruptEnable();
	sio2host_init();
	delay_ms(1000);
	SleepTimerInit();
	SystemTimerInit();
	AESInit();
	HAL_RadioInit();
	PDS_Init();
	PDS_DeleteAll();
	LORAWAN_Init(data_received, join_response);
	LORAWAN_Reset(ISM_EU868);
	SwTimerCreate(&timer);
	LORAWAN_SetAttr(DEV_EUI, &dev_eui[0]);
	LORAWAN_SetAttr(APP_EUI, &app_eui[0]);
	LORAWAN_SetAttr(APP_KEY, &app_key[0]);
	PDS_StoreAll();
	printf("\r\ntestapp initialized and configured\r\nstack version: %s", STACK_VER);
	LORAWAN_Join(LORAWAN_OTAA);
	SYSTEM_PostTask( APP_TASK_ID );
}

void join_response(StackRetStatus_t status) {
	if (LORAWAN_SUCCESS == status) {
		joined = true;
		printf("\r\njoined");
		SwTimerStart(timer, MS_TO_US(10000), SW_TIMEOUT_RELATIVE, timer_callback, NULL);
	}
	SYSTEM_PostTask(APP_TASK_ID);
}

void data_received(void *handle, appCbParams_t *params) {
	(void)handle;
	(void)params;
	if (params->evt == LORAWAN_EVT_RX_DATA_AVAILABLE) {
		printf("\r\ndownlink received");
	} else if (params->evt == LORAWAN_EVT_TRANSACTION_COMPLETE) {
		if (params->param.transCmpl.status == LORAWAN_SUCCESS) {
			printf("\r\ntx_ok");
		} else {
			printf("\r\ntx_not_ok");
		}
	}
}

void timer_callback(void) {
	timer_fired = true;
	SYSTEM_PostTask(APP_TASK_ID);
}

SYSTEM_TaskStatus_t APP_TaskHandler(void) {
	if (joined) {
		if (timer_fired) {
			timer_fired = false;
			SwTimerStart(timer, MS_TO_US(10000), SW_TIMEOUT_RELATIVE, timer_callback, NULL);
#ifdef UPLINK_ENABLED
			req.bufferLength = 1;
			req.buffer = &dev_eui[0];
			req.confirmed = LORAWAN_UNCNF;
			req.port = 1;
			printf("\r\nuplink sent status: %d", LORAWAN_Send(&req));
#endif
		}
	} else {
		LORAWAN_Join(LORAWAN_OTAA);
		printf("\r\njoining...");
	}
	return LORAWAN_SUCCESS;
}

void resources_init(void) {
	sio2host_init();
	HAL_RadioInit();
}

void resources_deinit(void) {
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.powersave  = true;
#ifdef HOST_SERCOM_PAD0_PIN
	port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
#endif
#ifdef HOST_SERCOM_PAD1_PIN
	port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
#endif
	sio2host_deinit();
	HAL_RadioDeInit();
}

PMM_Status_t sleep_start(void) {
	PMM_SleepReq_t sleepReq = {
		.sleepTimeMs = PMM_SLEEPTIME_MAX_MS,
		.pmmWakeupCallback = sleep_ended,
		.sleep_mode	= (HAL_SleepMode_t) CONF_PMM_SLEEPMODE_WHEN_IDLE
	};
	if (LORAWAN_ReadyToSleep(false)) {
		resources_deinit();		
		if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq)) {
			resources_init();
			SYSTEM_PostTask(APP_TASK_ID);
			return PMM_SLEEP_REQ_DENIED;
		}
	}
	return PMM_SLEEP_REQ_PROCESSED;
}

void sleep_ended(uint32_t slept_duration) {
	resources_init();
	SYSTEM_PostTask(APP_TASK_ID);
	printf("\r\nlast sleep time: %d ms", (unsigned int)slept_duration);
}
