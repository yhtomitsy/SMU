#include "Keypad.h"

/**@brief Initialize keypad parameters
 *
 */
 
void keypadBegin(uint8_t * key)
{	
		uint32_t err_code;
		key_ = key;
	  gpio_init();        // initialize GPIOs
		timer_initialize(); // initialize timers
		// start row scrolling timer for keypad
		err_code = app_timer_start(row_timer_id, APP_TIMER_TICKS(100), NULL);
    APP_ERROR_CHECK(err_code);
		NRF_LOG_INFO("Keypad started.");
		NRF_LOG_FLUSH();
}

/**@brief Button press behavior.
 *
 * @details This function is responsible for application of debounce and preventing wrong button presses
 * ToDo: make sure there are no false inputs. Might have to hardware debounce too
 */
static void button_press_behavior(uint8_t column)	
{
		uint32_t err_code;
		if(!keyPressed && !debounceFlag)
		{
				NRF_LOG_INFO("%c", keys[currentRow][column]);
				
				*key_ = keys[currentRow][column];
				
				keyPressed = 1;                          //  indicate that key is currently pressed
				err_code = app_timer_stop(row_timer_id); //  stop timer to prevent rows from cycling
								
				debounceFlag = 1; // set debounce flag to prevent uninteded button toggles
				err_code = app_timer_start(debounce_timer_id, APP_TIMER_TICKS(500), NULL); //  start debounce timer
				APP_ERROR_CHECK(err_code);
		}
		else
		{
				keyPressed = 0; // indicate that key is no longer pressed
		}
}

/**@brief Button event handler.
 *
 * @details This function is responsible for starting or stopping toggling of LED 1 based on button
 *          presses.
 *
 *          Print a log line indicating weather executing in thread/main or interrupt handler mode.
 *
 */
static void button_handler(nrf_drv_gpiote_pin_t pin)
{
    uint32_t err_code;
		NRF_LOG_INFO("Keypressed: %d", keyPressed);
	  NRF_LOG_INFO("Debounce flag: %d", debounceFlag);
    // Handle button press.
		switch (pin)
		{
			case COL1:
					button_press_behavior(0);
					break;
			case COL2:
					button_press_behavior(1);
					break;
			case COL3:
					button_press_behavior(2);
					break;
			default:
					break;
		}
}

/**@brief Button handler function to be called by the scheduler.
 */
void button_scheduler_event_handler(void *p_event_data, uint16_t event_size)
{
    // In this case, p_event_data is a pointer to a nrf_drv_gpiote_pin_t that represents
    // the pin number of the button pressed. The size is constant, so it is ignored.
    button_handler(*((nrf_drv_gpiote_pin_t*)p_event_data));
}


/**@brief Button event handler.
 */
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // The button_handler function could be implemented here directly, but is extracted to a
    // separate function as it makes it easier to demonstrate the scheduler with less modifications
    // to the code later in the tutorial.

    //button_handler(pin);
	  app_sched_event_put(&pin, sizeof(pin), button_scheduler_event_handler);
}


/**@brief Function for initializing GPIOs.
 */
static void gpio_init()
{
    ret_code_t err_code;

    // Initialze driver.
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
		
	  // Configure output pin for LED.
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
	  for(uint8_t i = 0; i < 4; i++)
		{
			err_code = nrf_drv_gpiote_out_init(pin_rows[i], &out_config);
			APP_ERROR_CHECK(err_code);
		}
	
		// set all the row pins high by detault
		for(uint8_t i = 0; i < 4; i++)
		{
			nrf_drv_gpiote_out_set(pin_rows[i]);
		}

    // Make a configuration for input pins. This is suitable for both pins in this example.
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    // Configure input pins for collumns of the keypad, with separate event handlers for each column
		for(uint8_t i = 0; i < 3; i++)
		{
			err_code = nrf_drv_gpiote_in_init(pin_column[i], &in_config, gpiote_event_handler);
			APP_ERROR_CHECK(err_code);
		}

    // Enable input pins for columns.
		for(uint8_t i = 0; i < 3; i++)
		{
			nrf_drv_gpiote_in_event_enable(pin_column[i], true);
    }
}


/**@brief Timeout handler for the repeated timer used for toggling of row pins
 *
 * @details Print a log line indicating weather executing in thread/main or interrupt mode.
 */
void timer_handler(void * p_context)
{
    
    nrf_drv_gpiote_out_set(pin_rows[currentRow]); // set previous row high
		if(currentRow < 3)
		{
			currentRow ++;
		}
		else 
		{
			currentRow = 0;
			debounceFlag = 0;                             // clear debounce flag
		}
		nrf_drv_gpiote_out_clear(pin_rows[currentRow]); // set current row low
		
    
		/*/ Log execution mode.
    if (current_int_priority_get() == APP_IRQ_PRIORITY_THREAD)
    {
        NRF_LOG_INFO("Timeout handler is executing in thread/main mode.");
    }
    else
    {
        NRF_LOG_INFO("Timeout handler is executing in interrupt handler mode.");
    }*/
}


/**@brief Timeout handler for the singleshot timer used for preventing debounce
 *
 * @details Print a log line indicating weather executing in thread/main or interrupt mode.
 */
void debounce_timer_handler(void * p_context)
{
		ret_code_t err_code;
	
		debounceFlag = 0; // clear debounce flag 
		NRF_LOG_INFO("Debounce ended");
		
	  err_code = app_timer_start(row_timer_id, APP_TIMER_TICKS(100), NULL); //  start timer to allow rows to cycle
		APP_ERROR_CHECK(err_code);
}

/**@brief Create timers.
 */
static void timer_initialize()
{
    uint32_t err_code;

    /*err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);*/

    err_code = app_timer_create(&row_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                timer_handler);
    APP_ERROR_CHECK(err_code);
	
	  err_code = app_timer_create(&debounce_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                debounce_timer_handler);
    APP_ERROR_CHECK(err_code);
}