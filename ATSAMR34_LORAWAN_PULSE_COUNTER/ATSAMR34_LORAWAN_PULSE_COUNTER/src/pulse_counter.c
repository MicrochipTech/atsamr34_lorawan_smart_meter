// m16946
/****************************** INCLUDES **************************************/
/*** INCLUDE FILES ************************************************************/
#include <asf.h>
#include "pulse_counter.h"
#include "conf_pulse_counter.h"

/*** SYMBOLIC CONSTANTS ********************************************************/

/*** GLOBAL VARIABLES & TYPE DEFINITIONS ***************************************/

/*** LOCAL VARIABLES & TYPE DEFINITIONS ****************************************/
static struct tcc_module tcc_capture_module ;
static struct tc_module tc_counter_module ;

/*** LOCAL FUNCTION PROTOTYPES *************************************************/
static void configure_extint(void) ;
static void configure_eventsystem(void) ;
static void configure_tcc(void) ;
static void configure_tc(void) ;
static void configure_sleep(void) ;

/*** GLOBAL FUNCTION DEFINITIONS ***********************************************/

void pulse_counter_init(void)
{
	configure_extint() ;
	configure_eventsystem() ;
#if (COUNTER_SELECTED == USE_TCC)
	configure_tcc() ;
#else
	configure_tc() ;
#endif	
	configure_sleep() ;
	
	PM->SLEEPCFG.reg = PM_SLEEPCFG_SLEEPMODE_STANDBY ;
	while(PM->SLEEPCFG.reg != PM_SLEEPCFG_SLEEPMODE_STANDBY) ;
}

uint32_t pulse_counter_getValue(void)
{
#if (COUNTER_SELECTED == USE_TCC)	
	return tcc_get_count_value(&tcc_capture_module) ;
#else
	return tc_get_count_value(&tc_counter_module) ;
#endif
}

/*******************************************************************************/

/*** LOCAL FUNCTION DEFINITIONS ************************************************/
// configure external interrupt for SW0
void configure_extint(void)
{
	// configure external interrupt controller
	struct extint_chan_conf extint_chan_config ;
	extint_chan_config.gpio_pin = CONF_EIC_PIN ;
	extint_chan_config.gpio_pin_mux = CONF_EIC_MUX ;
	extint_chan_config.gpio_pin_pull = EXTINT_PULL_UP ;
	extint_chan_config.detection_criteria = EXTINT_DETECT_RISING ;
	extint_chan_config.filter_input_signal = true ;
	extint_chan_set_config(CONF_EIC_CHAN, &extint_chan_config) ;
	
	// configure external interrupt module to be an event generator
	struct extint_events extint_event_config ;
	extint_event_config.generate_event_on_detect[CONF_EIC_CHAN] = true ;
	extint_enable_events(&extint_event_config) ;
}

// configure event system for generators and users
void configure_eventsystem(void)
{
	// configure event system
	struct events_resource event_res ;
	
	// configure channel
	struct events_config config ;
	events_get_config_defaults(&config) ;
	config.generator = CONF_EVENT_GENERATOR_ID ;
	config.edge_detect = EVENTS_EDGE_DETECT_RISING ;
	config.path = EVENTS_PATH_ASYNCHRONOUS ;
	config.run_in_standby = true ;
	events_allocate(&event_res, &config) ;
	
	// configure user
	events_attach_user(&event_res, CONF_EVENT_USER_ID) ;
}

// configure_tcc
void configure_tcc(void)
{
	// configure TCC module for capture
	static struct tcc_config tcc_conf ;
	tcc_reset(&tcc_capture_module) ;
	tcc_get_config_defaults(&tcc_conf, CONF_TCC) ;
	tcc_conf.counter.period = 0xFFFF ;
	tcc_conf.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1 ;
	tcc_conf.capture.channel_function[CONF_CAPTURE_CHAN_0] = TCC_CHANNEL_FUNCTION_CAPTURE ;
	tcc_conf.capture.channel_function[CONF_CAPTURE_CHAN_1] = TCC_CHANNEL_FUNCTION_CAPTURE ;
	tcc_conf.run_in_standby = true ;
	tcc_init(&tcc_capture_module, CONF_TCC, &tcc_conf) ;
	
	// configure TCC events
	struct tcc_events tcc_capture_events ;
	tcc_capture_events.on_input_event_perform_action[0] = true ;
	tcc_capture_events.input_config[0].modify_action = true ;
	tcc_capture_events.input_config[0].action = TCC_EVENT_ACTION_COUNT_EVENT ;
	tcc_enable_events(&tcc_capture_module, &tcc_capture_events) ;
	
	// enable TCC module
	tcc_enable(&tcc_capture_module) ;
}

// configure_tc
void configure_tc(void)
{
	// configure TC module for counting
	static struct tc_config tc_counter_config ;
	tc_reset(&tc_counter_module) ;
	tc_get_config_defaults(&tc_counter_config) ;
	tc_counter_config.clock_prescaler = TC_CLOCK_PRESCALER_DIV1 ;
	tc_counter_config.count_direction = TC_COUNT_DIRECTION_UP ;
	tc_counter_config.counter_size = TC_COUNTER_SIZE_16BIT ;
	tc_counter_config.on_demand = true ;
	tc_counter_config.run_in_standby = true ;
	tc_init(&tc_counter_module, CONF_TC, &tc_counter_config) ;

	struct tc_events tc_event = {
		.on_event_perform_action = true,
		.event_action = TC_EVENT_ACTION_INCREMENT_COUNTER,
		.generate_event_on_overflow = false
	} ;
	tc_enable_events(&tc_counter_module, &tc_event) ;
	// enable TC module
	tc_enable(&tc_counter_module) ;
}

// configure sleep
static void configure_sleep(void)
{
	/* Disable BOD33 */
	SUPC->BOD33.reg &= ~(SUPC_BOD33_ENABLE);

	/* Select BUCK converter as the main voltage regulator in active mode */
	SUPC->VREG.bit.SEL = SUPC_VREG_SEL_BUCK_Val;
	/* Wait for the regulator switch to be completed */
	while(!(SUPC->STATUS.reg & SUPC_STATUS_VREGRDY));

	/* Set Voltage Regulator Low power Mode Efficiency */
	SUPC->VREG.bit.LPEFF = 0x1;

	/* Apply SAM L21 Erratum 15264 */
	SUPC->VREG.bit.RUNSTDBY = 0x1;
	SUPC->VREG.bit.STDBYPL0 = 0x1;
	
	/* SRAM configuration in standby */
	PM->STDBYCFG.reg = PM_STDBYCFG_BBIASHS(1) | PM_STDBYCFG_VREGSMOD_LP ;
}
