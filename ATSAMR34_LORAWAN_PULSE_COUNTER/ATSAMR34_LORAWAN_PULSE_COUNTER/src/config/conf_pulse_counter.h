#ifndef CONF_PULSE_COUNTER_H
#define CONF_PULSE_COUNTER_H

// Counter selection
#define USE_TC	0
#define USE_TCC 1
#define COUNTER_SELECTED			USE_TC

// EXTINT Config
#define CONF_EIC_CHAN				BUTTON_0_EIC_LINE	// EXTINT8
#define CONF_EIC_PIN				BUTTON_0_EIC_PIN
#define CONF_EIC_MUX				BUTTON_0_EIC_MUX

// EVSYS Config
#define CONF_EVENT_GENERATOR_ID		EVSYS_ID_GEN_EIC_EXTINT_8
#if (COUNTER_SELECTED == USE_TCC)
  #define CONF_EVENT_USER_ID		EVSYS_ID_USER_TCC1_EV_0
#else
  #define CONF_EVENT_USER_ID		EVSYS_ID_USER_TC4_EVU	// must match with CONF_TC
#endif

// TCC
#define CONF_TCC					TCC1
#define CONF_CAPTURE_CHAN_0			0
#define CONF_CAPTURE_CHAN_1			1

// TC
#define CONF_TC						TC4 // TC0 is already used in LoRaWAN stack (hw_timer.c, conf_hw_timer.h)

#endif