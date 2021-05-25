#include <stdio.h>
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif

/* RTC i2C registers */
#define RTC_ADDR 0x68
#define Seconds_ADDR 0x00
#define Minutes_ADDR 0x01
#define Hour_ADDR 0x02
#define Day_ADDR 0x03
#define Date_ADDR 0x04
#define Month_ADDR 0x05
#define Year_ADDR 0x06
#define control_ADDR 0x07

static uint8_t sample[7] = {0};

/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/**
 * @brief Function for reading data from RTC.
 */
void readRtcData(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *date, uint8_t *month, uint8_t *year);

/**
 * @brief Function for writing data to RTC.
 */
static void writeRtcData(uint8_t reg_address, uint8_t data);

/**
 * @brief convert 8 bit value to BCD.
 */
static uint8_t byteToBCD(uint8_t val);

/**
 * @brief convert BCD to 8 bit value.
 */
uint8_t BCDToByte(uint8_t val);

/**
 * @brief Display RTC time
 */
//static void displayTime();

/**
 * @brief Set current time to RTC.
 */
static void setRtcTime (uint8_t hr, uint8_t mins, uint8_t secs, 
												uint8_t date, uint8_t month, uint8_t year);


/**
 * @brief TWI events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

/**
 * @brief UART initialization.
 */
void twi_init (void);

/**
 * @brief DS3231 initialization functions
 */
void RTC_begin(uint8_t sda, uint8_t scl, uint8_t hr, uint8_t mins, 
							 uint8_t secs, uint8_t date, uint8_t month, uint8_t year);
	
static uint8_t SDA_pin = 0;
static uint8_t SCL_pin = 0;
