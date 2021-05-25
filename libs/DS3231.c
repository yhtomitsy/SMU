#include "DS3231.h"

/**
 * @brief Function for reading data from RTC.
 */
void readRtcData(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *date, uint8_t *month, uint8_t *year)
{
		m_xfer_done = false; // active transaction
	  ret_code_t err_code;

    /* Writing to LM75B_REG_CONF "0" set temperature sensor in NORMAL mode. */
    uint8_t reg[1] = {0};
    err_code = nrf_drv_twi_tx(&m_twi, RTC_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
		
		m_xfer_done = false;
		
		/* Read 7 bytes from the specified address */
    err_code = nrf_drv_twi_rx(&m_twi, RTC_ADDR, sample, 7);
    APP_ERROR_CHECK(err_code);
		
		while (m_xfer_done == false);
		
		*seconds = BCDToByte(sample[0]);
		*minutes = BCDToByte(sample[1]);
		*hours = BCDToByte(sample[2]);
		*date = BCDToByte(sample[4]);
		*month = BCDToByte(sample[5]);
		*year = BCDToByte(sample[6]);
}

/**
 * @brief Function for writing data to RTC.
 */
static void writeRtcData(uint8_t reg_address, uint8_t data)
{
		m_xfer_done = false; // active transaction
	  
	  ret_code_t err_code;

    /* Writing to RTC*/
    uint8_t reg[2] = {reg_address, data};
		err_code = nrf_drv_twi_tx(&m_twi, RTC_ADDR, reg, sizeof(reg), false);
		APP_ERROR_CHECK(err_code);
		
    while (m_xfer_done == false)
		{
				nrf_delay_ms(1);
				/*NRF_LOG_INFO(".");
				NRF_LOG_FLUSH();*/
		}
}

/**
 * @brief convert 8 bit value to BCD.
 */
static uint8_t byteToBCD(uint8_t val)
{
		return (val / 10 << 4) | (val % 10);
}

/**
 * @brief convert BCD to 8 bit value.
 */
uint8_t BCDToByte(uint8_t val)
{
		return (10 * (val >> 4))+ (val & 0xf);
}

/**
 * @brief Set current time to RTC.
 */
static void setRtcTime (uint8_t hr, uint8_t mins, uint8_t secs, 
												uint8_t date, uint8_t month, uint8_t year)
{
		writeRtcData(Hour_ADDR, byteToBCD(hr));
		writeRtcData(Minutes_ADDR, byteToBCD(mins));
		writeRtcData(Seconds_ADDR, byteToBCD(secs));
		writeRtcData(Date_ADDR, byteToBCD(date));
		writeRtcData(Month_ADDR, byteToBCD(month));
		writeRtcData(Year_ADDR, byteToBCD(year));
} 

/**
 * @brief TWI events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                //data_handler(m_sample);
								//displayTime();
            }
            m_xfer_done = true;
						//NRF_LOG_INFO("i2Cevent done");
            break;
        default:
            break;
    }
}

/**
 * @brief UART initialization.
 */
void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_RTC_config = {
       .scl                = SCL_pin,//ARDUINO_SCL_PIN,
       .sda                = SDA_pin,//ARDUINO_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_RTC_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

/**
 * @brief DS3231 initialization functions
 */
void RTC_begin(uint8_t sda, uint8_t scl, uint8_t hr, uint8_t mins, 
							 uint8_t secs, uint8_t date, uint8_t month, uint8_t year)
{
		
    SCL_pin = scl;
		SDA_pin = sda;
	
		twi_init();
		
		setRtcTime (hr, mins, secs, date, month, year);
	
		NRF_LOG_INFO("RTC ok.");
    NRF_LOG_FLUSH();
}
