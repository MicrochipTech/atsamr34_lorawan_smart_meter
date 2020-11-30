// m16946
/****************************** INCLUDES **************************************/
/*** INCLUDE FILES ************************************************************/
#include <asf.h>
#include "lorawan_app.h"
#include "system_low_power.h"
#include "radio_driver_hal.h"
#include "lorawan.h"
#include "sys.h"
#include "system_init.h"
#include "system_assert.h"
#include "aes_engine.h"
#include "sio2host.h"
#include "sw_timer.h"
#ifdef CONF_PMM_ENABLE
  #include "pmm.h"
  #include  "conf_pmm.h"
  #include "sleep_timer.h"
  #include "sleep.h"
#endif
#include "conf_sio2host.h"
#if (ENABLE_PDS == 1)
  #include "pds_interface.h"
#endif
#ifdef CRYPTO_DEV_ENABLED
  #include "sal.h"
#endif

#include "resources.h"
#include "temp_sensor.h"
#include "led.h"
#include "utils.h"
#include "pulse_counter.h"
#include "conf_sal.h"
#include "cryptoauthlib.h"

/*** SYMBOLIC CONSTANTS ********************************************************/
#define APP_SLEEP_TIME_MS						PMM_SLEEPTIME_MAX_MS
#define INITIAL_PERIODIC_UPLINK_TIMEOUT			10000
#define PERIODIC_UPLINK_TIMEOUT					60000	// Periodic uplink in ms
#define JOIN_REQUEST_LED_TIMEOUT				500
#define JOIN_ACCEPT_LED_TIMEOUT					1000
#define SUBBAND									2		// TTN use sub band #2
#define APP_PORT								1		// LoRaWAN Application Port
#define AVG_CNT									20		// Number of A/D samples

/*** GLOBAL VARIABLES & TYPE DEFINITIONS ***************************************/
// LEDs states
static uint8_t on = LON ;
static uint8_t off = LOFF ;
static uint8_t appLedState ;
static uint8_t statusLedState ;
// Application Timer ID
uint8_t ledTimerID = 0xFF ;
uint8_t periodicUplinkTimerID = 0xFF ;
volatile bool periodicUplinkTimerFired = false ;
uint32_t periodicUplinkTimeout = INITIAL_PERIODIC_UPLINK_TIMEOUT ;

// LoRaWAN
bool sleepEnable = true ;
bool joined = false ;
LorawanSendReq_t lorawanSendReq ;
uint8_t lorawanPayload[20] = {0} ;

// hard-coded OTAA credentials
// Tested with TTI Network Server (MAC 1.04, PHY 1.01)
uint8_t demoDevEui[8] =  {0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA} ; //{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } ;
uint8_t demoAppEui[8] =  {0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA} ; //{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } ;
uint8_t demoAppKey[16] = {0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA} ; //{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } ;

#ifdef CONF_PMM_ENABLE
bool deviceResetsForWakeup = false;
#endif

/*** LOCAL FUNCTION PROTOTYPES *************************************************/
static void configure_subband(uint8_t subband) ;
static void appWakeup(uint32_t sleptDuration) ;
static void app_resources_init(void) ;
static void app_resources_deinit(void) ;
static void print_lorawan_parameters(void) ;
static void joindata_callback(StackRetStatus_t status) ;
static void appdata_callback(void *appHandle, appCbParams_t *appdata) ;
static void periodicUplink_callback(void) ;
static void ledTimer_callback(void) ;
static void periodicSend(void) ;
static void send_data(void) ;
static inline void set_statusLED_Off() { set_LED_data(LED_AMBER, &off) ; statusLedState = false ; }
static inline void set_statusLED_On() { set_LED_data(LED_AMBER, &on) ; statusLedState = true ; }
static inline void set_appLED_Off() { set_LED_data(LED_GREEN, &off) ; statusLedState = false ; }
static inline void set_appLED_On() { set_LED_data(LED_GREEN, &on) ; statusLedState = true ; }
static inline uint8_t get_statusLED() { return statusLedState ; }
static inline uint8_t get_appLED() { return appLedState ; }
static float convert_celsius_to_fahrenheit(float celsius_val) ;
#ifdef CRYPTO_DEV_ENABLED
  static void print_ecc_info(void) ;
#endif

/*** GLOBAL FUNCTION DEFINITIONS ***********************************************/

/*** lorawan_app_init ***********************************************************
 \brief      LoRaWAN Stack Driver Init.
********************************************************************************/
void lorawan_app_init(void)
{
	printf("\r\n--- ATSAMR34_LORAWAN_PULSE_COUNTER ---\r\n") ;
	set_statusLED_Off() ;
	set_appLED_Off() ;
	resource_init() ;
	HAL_RadioInit() ;
#ifndef CRYPTO_DEV_ENABLED	
	AESInit() ;
#else
	if (SAL_SUCCESS != SAL_Init())
	{
		printf("Initialization of Security module is failed\r\n");
		/* Stop Further execution */
		while (1) {
		}		
	}
	print_ecc_info() ;
#endif
	SystemTimerInit() ;
#ifdef CONF_PMM_ENABLE
	SleepTimerInit() ;
#endif
#if (ENABLE_PDS == 1)
	PDS_Init() ;
	//PDS_DeleteAll() ;
#endif
	Stack_Init() ;
	
	SwTimerCreate(&ledTimerID) ;
	//SwTimerCreate(&periodicUplinkTimerID) ;
	//SwTimerStart(periodicUplinkTimerID, MS_TO_US(60000), SW_TIMEOUT_RELATIVE, (void*)periodicUplink_callback, NULL) ;
	
}

/*** lorawan_app_configuration **************************************************
 \brief      LoRaWAN Application Configuration
********************************************************************************/
void lorawan_app_configuration(IsmBand_t bandSelection)
{
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST ;
	LORAWAN_Init(appdata_callback, joindata_callback) ;
	LORAWAN_Reset(bandSelection) ;
	uint8_t datarate ;
#if (NA_BAND == 1 || AU_BAND == 1)
	if (bandSelection == ISM_NA915 || bandSelection == ISM_AU915)
	{
		configure_subband(bandSelection) ;
		datarate = DR3 ;
	}
#endif

#if (EU_BAND == 1)
	if (bandSelection == ISM_EU868)
	{
		datarate = DR5 ;
	}
#endif
	status = LORAWAN_SetAttr(CURRENT_DATARATE, &datarate) ;

#ifdef CRYPTO_DEV_ENABLED
	bool cryptoDevEnabled = true ;
	status = LORAWAN_SetAttr(CRYPTODEVICE_ENABLED, &cryptoDevEnabled) ;
#else
	status = LORAWAN_SetAttr(DEV_EUI, demoDevEui) ;
	status = LORAWAN_SetAttr(JOIN_EUI, demoAppEui) ;
	status = LORAWAN_SetAttr(APP_KEY, demoAppKey) ;
#endif

	EdClass_t classType = CLASS_A ;
	status = LORAWAN_SetAttr(EDCLASS, &classType) ;

	bool adr = true ;
	status = LORAWAN_SetAttr(ADR, &adr) ;
	
	bool joinBackoffEnable = false ;
	LORAWAN_SetAttr(JOIN_BACKOFF_ENABLE, &joinBackoffEnable) ;

#if (ENABLE_PDS == 1)
	PDS_StoreAll() ;
#endif
	
#ifndef CRYPTO_DEV_ENABLED
	print_lorawan_parameters() ;
#endif

}

/*** lorawan_app_join ***********************************************************
 \brief      LoRaWAN Application Activation
********************************************************************************/
bool lorawan_app_join(void)
{
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST ;
	bool ret = false ;
	SwTimerStart(ledTimerID, MS_TO_US(JOIN_REQUEST_LED_TIMEOUT), SW_TIMEOUT_RELATIVE, (void*)ledTimer_callback, NULL) ;
	joined = false ;
	status = LORAWAN_Join(LORAWAN_OTAA) ;
	printf("\r\n***************Device Activation***************\r\n");
	if (LORAWAN_SUCCESS == status)
	{
		printf("Join Request sent to the network server...\r\n") ;
		ret = true ;
	}
	return ret ;
}

/*** lorawan_app_transmit *******************************************************
 \brief      Function to transmit an uplink message
 \param[in]  type	- transmission type (LORAWAN_UNCNF or LORAWAN_CNF)
 \param[in]  fport	- 1-255
 \param[in]  data	- buffer to transmit
 \param[in]  len	- buffer length
********************************************************************************/
void lorawan_app_transmit(TransmissionType_t type, uint8_t fport, void* data, uint8_t len)
{
	if (joined)
	{
		lorawanSendReq.buffer = data ;
		lorawanSendReq.bufferLength = len ;
		lorawanSendReq.confirmed = type ;	// LORAWAN_UNCNF or LORAWAN_CNF
		lorawanSendReq.port = fport ;		// fport [1-255]
		printf("Uplink message sent status: %d\r\n", LORAWAN_Send(&lorawanSendReq)) ;
		set_statusLED_On() ;
	}
}

/*** lorawan_app_run_tasks ******************************************************
 \brief      LoRaWAN Run Tasks
********************************************************************************/
void lorawan_app_run_tasks(void)
{
	SYSTEM_RunTasks() ;
}

/*** lorawan_app_sleep **********************************************************
 \brief      Put the LoRaWAN Application to sleep
********************************************************************************/
PMM_Status_t lorawan_app_sleep(void)
{
#ifdef CONF_PMM_ENABLE
if (sleepEnable)
{
	PMM_SleepReq_t sleepReq = {
		.sleepTimeMs = periodicUplinkTimeout,
		.pmmWakeupCallback = appWakeup,
		.sleep_mode = (HAL_SleepMode_t) CONF_PMM_SLEEPMODE_WHEN_IDLE
	} ;
	if (CONF_PMM_SLEEPMODE_WHEN_IDLE == SLEEP_MODE_STANDBY)
	{
		deviceResetsForWakeup = false ;
	}
	if (true == LORAWAN_ReadyToSleep(deviceResetsForWakeup))
	{
		app_resources_deinit() ;
		if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq))
		{
			app_resources_init() ;
			return PMM_SLEEP_REQ_DENIED ;
			/*printf("\r\nsleep_not_ok\r\n");*/
		}
	}
}
	return PMM_SLEEP_REQ_PROCESSED ;
#endif	
}

/*** lorawan_app_wakeup *********************************************************
 \brief      Wake up LoRaWAN App from Sleep
********************************************************************************/
void lorawan_app_wakeup(void)
{
#ifdef CONF_PMM_ENABLE
	PMM_Wakeup() ;
#endif
}

/*** lorawan_app_setDevEui ******************************************************
 \brief      Set LoRaWAN DevEUI
********************************************************************************/
void lorawan_app_setDevEui(const uint8_t *devEui, uint8_t devEuiLen)
{
	memcpy(demoDevEui, devEui, devEuiLen) ;
}

/*** lorawan_app_setAppEui ******************************************************
 \brief      Set LoRaWAN AppEUI
********************************************************************************/
void lorawan_app_setAppEui(const uint8_t *appEui, uint8_t appEuiLen)
{
	memcpy(demoAppEui, appEui, appEuiLen) ;
}

/*** lorawan_app_setAppKey ******************************************************
 \brief      Set LoRaWAN AppKey
********************************************************************************/
void lorawan_app_setAppKey(const uint8_t *appKey, uint8_t appKeyLen)
{
	memcpy(demoAppKey, appKey, appKeyLen) ;
}

/*******************************************************************************/

/*** LOCAL FUNCTION DEFINITIONS ************************************************/

/*** configure_subband **********************************************************
 \brief      Configure Frequency Sub Band FSB
********************************************************************************/
static void configure_subband(uint8_t subband)
{
#if (NA_BAND == 1 || AU_BAND == 1)
#if (RANDOM_NW_ACQ == 0)
	//printf("Configure sub band: %d\r\n", subband) ;
	#define MAX_NA_CHANNELS 72
	#define MAX_SUBBAND_CHANNELS 8

	ChannelParameters_t ch_params ;
	uint8_t allowed_min_125khz_ch, allowed_max_125khz_ch, allowed_500khz_channel ;
	allowed_min_125khz_ch = (subband-1)*MAX_SUBBAND_CHANNELS ;
	allowed_max_125khz_ch = ((subband-1)*MAX_SUBBAND_CHANNELS) + 7 ;
	allowed_500khz_channel = subband+63 ;
	for (ch_params.channelId = 0; ch_params.channelId < MAX_NA_CHANNELS; ch_params.channelId++)
	{
		if((ch_params.channelId >= allowed_min_125khz_ch) && (ch_params.channelId <= allowed_max_125khz_ch))
		{
			ch_params.channelAttr.status = true ;
		}
		else if(ch_params.channelId == allowed_500khz_channel)
		{
			ch_params.channelAttr.status = true ;
		}
		else
		{
			ch_params.channelAttr.status = false ;
		}
		LORAWAN_SetAttr(CH_PARAM_STATUS, &ch_params) ;
	}
#endif
#endif
}

static void switch_main_dfll(void)
{
	struct system_gclk_gen_config gclk_conf;
	struct system_gclk_chan_config dfll_gclk_chan_conf;
	system_gclk_chan_get_config_defaults(&dfll_gclk_chan_conf);
	dfll_gclk_chan_conf.source_generator = GCLK_GENERATOR_1;
	system_gclk_chan_set_config(OSCCTRL_GCLK_ID_DFLL48, &dfll_gclk_chan_conf);
	system_gclk_chan_enable(OSCCTRL_GCLK_ID_DFLL48);
	struct system_clock_source_dfll_config dfll_conf;
	system_clock_source_dfll_get_config_defaults(&dfll_conf);
	dfll_conf.loop_mode      = SYSTEM_CLOCK_DFLL_LOOP_MODE_CLOSED;
	dfll_conf.on_demand      = false;
	dfll_conf.run_in_stanby  = CONF_CLOCK_DFLL_RUN_IN_STANDBY;
	dfll_conf.multiply_factor = CONF_CLOCK_DFLL_MULTIPLY_FACTOR;
	system_clock_source_dfll_set_config(&dfll_conf);
	system_clock_source_enable(SYSTEM_CLOCK_SOURCE_DFLL);
	while(!system_clock_source_is_ready(SYSTEM_CLOCK_SOURCE_DFLL));
	if (CONF_CLOCK_DFLL_ON_DEMAND) {
		OSCCTRL->DFLLCTRL.bit.ONDEMAND = 1;
	}
	/* Select DFLL for mainclock. */
	system_gclk_gen_get_config_defaults(&gclk_conf);
	gclk_conf.source_clock = SYSTEM_CLOCK_SOURCE_DFLL;
	gclk_conf.division_factor= CONF_CLOCK_GCLK_0_PRESCALER;
	system_gclk_gen_set_config(GCLK_GENERATOR_0, &gclk_conf);
}

/*** appWakeup ******************************************************************
 \brief      Application Wake Up
 \param[in]  sleptDuration - slept duration in ms
********************************************************************************/
#ifdef CONF_PMM_ENABLE
static void appWakeup(uint32_t sleptDuration)
{
	switch_main_dfll() ;
	app_resources_init() ;
	printf("\r\nsleep_ok %ld ms\r\n", sleptDuration) ;
	periodicUplinkTimeout = PERIODIC_UPLINK_TIMEOUT ;
	periodicSend() ;
}
#endif

#ifdef CONF_PMM_ENABLE
/*** app_resources_init *********************************************************
 \brief      Init. the application resources
********************************************************************************/
static void app_resources_init(void)
{
	HAL_Radio_resources_init() ;
	sio2host_init() ;
}

/*** app_resources_uninit *******************************************************
 \brief      Uninit. the application resources
********************************************************************************/
static void app_resources_deinit(void)
{
	// Disable USART TX and RX Pins
	struct port_config pin_conf ;
	port_get_config_defaults(&pin_conf) ;
	pin_conf.powersave  = true ;
	port_pin_set_config(PIN_PA04D_SERCOM0_PAD0, &pin_conf) ;
	port_pin_set_config(PIN_PA05D_SERCOM0_PAD1, &pin_conf) ;
	// Disable UART module
	sio2host_deinit() ;
	// Disable Transceiver SPI Module
	HAL_RadioDeInit() ;
}
#endif

static void print_lorawan_parameters(void)
{
	IsmBand_t ismBand ;
	uint8_t dataRate ;
	uint8_t devEui[8] = {0} ;
	uint8_t appEui[8] = {0} ;
	uint8_t appKey[16] = {0} ;	
	EdClass_t edClass ;
	printf("***************Application Configuration***************\r\n") ;
	LORAWAN_GetAttr(ISMBAND, (void*)NULL, &ismBand) ;
	if (ismBand == ISM_NA915)
	{
		printf("ISM Band: ISM_NA915\r\n") ;
		printf("Sub Band: %d\r\n", SUBBAND) ;
	}
	else if (ismBand == ISM_EU868)
	{
		printf("ISM Band: ISM_EU868\r\n") ;
	}
	LORAWAN_GetAttr(CURRENT_DATARATE, (void*)NULL, &dataRate) ;
	printf("Data Rate: %d\r\n", dataRate) ;
	LORAWAN_GetAttr(DEV_EUI, (void*)NULL, &devEui) ;
	printf("DevEui: ") ;
	print_array(devEui, sizeof(devEui)) ;
	LORAWAN_GetAttr(JOIN_EUI, (void*)NULL, &appEui) ;
	printf("AppEui: ") ;
	print_array(appEui, sizeof(appEui)) ;
	LORAWAN_GetAttr(APP_KEY, (void*)NULL, &appKey) ;
	printf("AppKey: ") ;
	print_array(appKey, sizeof(appKey)) ;		
	LORAWAN_GetAttr(EDCLASS, (void*)NULL, &edClass) ;
	printf("DevType: ") ;
    if(edClass == CLASS_A)
    {
	    printf("CLASS A\r\n");
    }
    else if(edClass == CLASS_C)
    {
	    printf("CLASS C\r\n");
    }
	printf("\r\n") ;
}

/*** joindata_callback **********************************************************
 \brief      Callback function for the ending of Activation procedure
 \param[in]  status - join status
********************************************************************************/
static void joindata_callback(StackRetStatus_t status)
{
#ifdef CRYPTO_DEV_ENABLED
	printf("DevEUI: ") ;
	LORAWAN_GetAttr(DEV_EUI, NULL, &demoDevEui) ;
	print_array((uint8_t *)&demoDevEui, sizeof(demoDevEui)) ;
#endif

	// This is called every time the join process is finished
	if(LORAWAN_SUCCESS == status)
	{
		joined = true ;
		set_statusLED_Off() ;
		SwTimerStop(ledTimerID) ;
		printf("Join Successful!\r\n") ;
		PDS_StoreAll() ;
	}
	else if(LORAWAN_NO_CHANNELS_FOUND == status)
	{
		joined = false ;
		set_statusLED_Off() ;
		printf("No Free Channel found\r\n") ;
	}
	else if (LORAWAN_MIC_ERROR == status)
	{
		joined = false ;
		set_statusLED_Off() ;
		printf("MIC Error\r\n") ;
	}
	else if (LORAWAN_TX_TIMEOUT == status)
	{
		joined = false ;
		set_statusLED_Off() ;
		printf("Transmission Timeout\r\n") ;
	}
	else
	{
		joined = false ;
		set_statusLED_Off() ;
		printf("Join Denied\r\n") ;
	}

	if (joined == false)
	{
		lorawan_app_join() ;
	}
}

/*** appdata_callback ***********************************************************
 \brief      Callback function for the ending of Bidirectional communication of
			 Application data
 \param[in]  *appHandle - callback handle
 \param[in]  *appData - callback parameters
********************************************************************************/
static void appdata_callback(void *appHandle, appCbParams_t *appdata)
{
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST;

	if (LORAWAN_EVT_RX_DATA_AVAILABLE == appdata->evt)
	{
		status = appdata->param.rxData.status;
		switch(status)
		{
			case LORAWAN_SUCCESS:
			{
				uint8_t *pData = appdata->param.rxData.pData ;
				uint8_t dataLength = appdata->param.rxData.dataLength ;
				if((dataLength > 0U) && (NULL != pData))
				{
					printf("*** Received DL Data ***\n\r") ;
					printf("\nFrame Received at port %d\n\r", pData[0]) ;
					printf("\nFrame Length - %d\n\r", dataLength) ;
					printf ("\nPayload: ") ;
					for (uint8_t i = 0; i < dataLength - 1; i++)
					{
						printf("%0x", pData[i+1]) ;
					}
					printf("\r\n*************************\r\n") ;
				}
				else
				{
					printf("Received ACK for Confirmed data\r\n") ;
				}
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("\n\rNO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			case LORAWAN_INVALID_PACKET:
			printf("\n\rINVALID_PACKET \n\r");
			break;
			default:
			printf("UNKNOWN ERROR\n\r");
			break;
		}
	}
	else if(LORAWAN_EVT_TRANSACTION_COMPLETE == appdata->evt)
	{
		switch(status = appdata->param.transCmpl.status)
		{
			case LORAWAN_SUCCESS:
			{
				printf("> Transmission Success\r\n") ;
				set_statusLED_Off() ;
			}
			break;
			case LORAWAN_RADIO_SUCCESS:
			{
				printf("> Transmission Success\r\n");
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_TX_TIMEOUT:
			printf("\nTx Timeout\n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("> NO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			case LORAWAN_INVALID_PACKET:
			printf("\n\rINVALID_PACKET \n\r");
			break;
			default:
			printf("\n\rUNKNOWN ERROR\n\r");
			break;
		}
		uint32_t pendingDC = 0 ;
		LORAWAN_GetAttr(PENDING_DUTY_CYCLE_TIME, NULL, &pendingDC) ;
		printf("Pending duty cycle time: %ld ms", pendingDC) ;
		printf("\n\r*************************************************\n\r");
	}
	sleepEnable = true ;
}

static void ledTimer_callback(void)
{
	if (joined == false)
	{
		if (get_statusLED() == 1)
		{
			set_statusLED_Off() ;
		}
		else
		{
			set_statusLED_On() ;
		}
		SwTimerStart(ledTimerID, MS_TO_US(JOIN_REQUEST_LED_TIMEOUT), SW_TIMEOUT_RELATIVE, (void*)ledTimer_callback, NULL) ;
	}
}

static void periodicUplink_callback(void)
{
	periodicUplinkTimerFired = true ;
	SYSTEM_PostTask(APP_TASK_ID) ;
}

static void periodicSend(void)
{
	periodicUplinkTimerFired = true ;
	sleepEnable = false ;
	SYSTEM_PostTask(APP_TASK_ID) ;	
}

/*** APP_TaskHandler ************************************************************
 \brief      Application task handler (has to be declared global)
********************************************************************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void)
{
	if (joined)
	{
		if (periodicUplinkTimerFired)
		{
			periodicUplinkTimerFired = false ;
			send_data() ;
		}
	}
	return SYSTEM_TASK_SUCCESS ;
}

static void send_data(void)
{
	float c_val ;
	float f_val ;
	int c_avg_val = 0 ;
	for (uint8_t i = 0; i < AVG_CNT; i++)
	{
		get_temp_sensor_data((uint8_t*)&c_val) ;
		c_avg_val = c_avg_val + c_val ;
	}
	c_val = c_avg_val / AVG_CNT ;
	f_val = convert_celsius_to_fahrenheit(c_val) ;
	printf("\r\nPulse Counter value: %lu", pulse_counter_getValue()) ;
	snprintf(lorawanPayload, sizeof(lorawanPayload), "%lu/%.1fC/%.1fF", pulse_counter_getValue(), c_val, f_val) ;
	printf("\r\nPayload to transmit: %s [%d]", lorawanPayload, strlen(lorawanPayload)) ;
	static uint32_t frameCounter = 0 ;
	LORAWAN_GetAttr(UPLINK_COUNTER, NULL, &frameCounter) ;
	printf("\r\nFrame Counter value: %ld\r\n", frameCounter) ;
	lorawan_app_transmit(LORAWAN_UNCNF, APP_PORT, lorawanPayload, strlen(lorawanPayload)) ;		
}

/*** convert_celsius_to_fahrenheit **********************************************
 \brief      Function to convert Celsius value to Fahrenheit
 \param[in]  cel_val    - temperature value in Celsius
 \param[out] fahren_val - temperature value in Fahrenheit
********************************************************************************/
static float convert_celsius_to_fahrenheit(float celsius_val)
{
    float fahren_val ;
    /* T(°F) = T(°C) × 9/5 + 32 */
    fahren_val = (((celsius_val * 9)/5) + 32) ;
    return fahren_val ;
}

#ifdef CRYPTO_DEV_ENABLED
static void print_ecc_info(void)
{
	ATCA_STATUS  status ;
	uint8_t    sn[9] ;			// ECC608A serial number (9 Bytes)
	uint8_t    info[2] ;
	uint8_t    tkm_info[10] ;
	int      slot = 10 ;
	int      offset = 70 ;
	uint8_t appEUI[8] ;
	uint8_t devEUIascii[16] ;
	uint8_t devEUIdecoded[8] ;	// hex.
	size_t bin_size = sizeof(devEUIdecoded) ;

	// read the serial number
	status = atcab_read_serial_number(sn) ;
	printf("\r\n--------------------------------\r\n") ;

	// read the SE_INFO
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, slot, offset, info, sizeof(info)) ;
	
	// Read the CustomDevEUI
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, DEV_EUI_SLOT, 0, devEUIascii, 16) ;
	atcab_hex2bin((char*)devEUIascii, strlen((char*)devEUIascii), devEUIdecoded, &bin_size) ;

	printf("ECC608A Secure Element:\r\n") ;

	// Print DevEUI
	printf("DEV EUI       ") ;
	#if (SERIAL_NUM_AS_DEV_EUI == 1)
	print_array(sn, sizeof(sn)-1) ;
	#else
	print_array(devEUIdecoded, sizeof(devEUIdecoded)) ;
	#endif
	
	// Read the AppEUI
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, APP_EUI_SLOT, 0, appEUI, 8) ;
	printf("JOIN EUI      ") ;
	print_array(appEUI, sizeof(appEUI)) ;
	
	// assemble full TKM_INFO
/*	memcpy(tkm_info, info, 2) ;
	memcpy(&tkm_info[2], sn, 8) ;
	// tkm_info[] now contains the assembled tkm_info data
	printf("TKM INFO: ") ;
	print_array(tkm_info, sizeof(tkm_info)) ;
*/
	printf("SERIAL NUMBER ") ;
	print_array(sn, sizeof(sn)) ;
	
	
	printf("--------------------------------\r\n") ;
}
#endif