#include <stdbool.h>
#include <stdint.h>
#include "boards.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_scheduler.h"
#include "nrf_delay.h"

APP_TIMER_DEF(row_timer_id);
APP_TIMER_DEF(debounce_timer_id);

/*Keypad*/
#define COL1 13
#define COL2 12
#define COL3 11

#define ROW_NUM 4    //four rows
#define COLUMN_NUM 3 //three columns

static uint8_t keys[ROW_NUM][COLUMN_NUM] = {
  {1,2,3},
  {4,5,6},
  {7,8,9},
  {'*',0,'#'}
};

static uint8_t pin_rows[ROW_NUM] = {17, 16, 15, 14};  //connect to the row pinouts of the keypad
static uint8_t pin_column[COLUMN_NUM] = {COL1, COL2, COL3}; //connect to the column pinouts of the keypad
static uint8_t volatile currentRow = 0;                        //hold current row that is being executed
static uint8_t volatile keyPressed = 0;                      // 1 if button has been presed in the cycle and 0 if not
static uint8_t volatile debounceFlag = 0;                      // 1 if button has been presed in the cycle and 0 if not
static uint8_t volatile *key_;                                 // will hold address of the variable that stores the currently pressed button

/**@brief Initialize keypad parameters
 *
 */
 
void keypadBegin(uint8_t * key);

static void button_press_behavior(uint8_t column);

static void button_handler(nrf_drv_gpiote_pin_t pin);

void button_scheduler_event_handler(void *p_event_data, uint16_t event_size);

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

static void gpio_init();

void timer_handler(void * p_context);

void debounce_timer_handler(void * p_context);

static void timer_initialize();