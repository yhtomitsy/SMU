#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "Small_7.h"

static uint8_t DC_PIN = 3;
static uint8_t RST_PIN = 4;
static uint8_t CS_PIN = SPI_SS_PIN;
static uint8_t SDI_PIN = SPI_MOSI_PIN;
static uint8_t SCK_PIN = SPI_SCK_PIN;

#define LCD_FB_SIZE             1024
#define MAX_PIXEL_X             128
#define MAX_PIXEL_Y             64

static unsigned char buffer[LCD_FB_SIZE];
static uint8_t auto_up;
static unsigned int char_x = 0;
static unsigned int char_y;
static unsigned char* font;
static unsigned int contrast = 0;
static unsigned int orientation = 1;

/** Draw mode
  * NORMAl
  * XOR set pixel by xor the screen
  */
enum {LCD_NORMAL, LCD_XOR};
static unsigned int draw_mode = LCD_NORMAL;

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

#define TEST_STRING "Nordic"
static uint8_t       m_tx_buf[] = TEST_STRING;           /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];    /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

		int width();
 
		/** Get the height of the screen in pixel
		*
     * @returns height of screen in pixel
     *
     */
    int height();
 
    /** Draw a pixel at x,y black or white
     *
     * @param x horizontal position
     * @param y vertical position
     * @param colour ,1 set pixel ,0 erase pixel
     */
    void pixel(int x, int y,int colour);
 
    /** draw a circle
      *
      * @param x0,y0 center
      * @param r radius
      * @param colour ,1 set pixel ,0 erase pixel
      *
      */
    void circle(int x, int y, int r, int colour);
 
    /** draw a filled circle
     *
     * @param x0,y0 center
     * @param r radius
     * @param color ,1 set pixel ,0 erase pixel
     *
     * use circle with different radius,
     * can miss some pixel
     */
    void fillcircle(int x, int y, int r, int colour);
 
    /** draw a 1 pixel line
      *
      * @param x0,y0 start point
      * @param x1,y1 stop point
      * @param color ,1 set pixel ,0 erase pixel
      *
      */
    void line(int x0, int y0, int x1, int y1, int colour);
 
    /** draw a rect
    *
    * @param x0,y0 top left corner
    * @param x1,y1 down right corner
    * @param color 1 set pixel ,0 erase pixel
    *                                                   *
    */
    void rect(int x0, int y0, int x1, int y1, int colour);
 
    /** draw a filled rect
      *
      * @param x0,y0 top left corner
      * @param x1,y1 down right corner
      * @param color 1 set pixel ,0 erase pixel
      *
      */
    void fillrect(int x0, int y0, int x1, int y1, int colour);
 
    /** copy display buffer to lcd
      *
      */
 
    void copy_to_lcd(void);
 
    /** set the orienation of the screen
      *
      */
 
 
    void set_contrast(unsigned int o);
 
    /** read the contrast level
      *
      */
    unsigned int get_contrast(void);
 
 
    /** invert the screen
      *
      * @param o = 0 normal, 1 invert
      */
    void invert(unsigned int o);
 
    /** clear the screen
       *
       */
    void cls(void);
 
    /** set the drawing mode
      *
      * @param mode NORMAl or XOR
      */
 
    void setmode(int mode);
 
    int columns(void);
 
    /** calculate the max number of columns
     *
     * @returns max column
     * depends on actual font size
     *
     */
    int rows(void);
 
    /** put a char on the screen
     *
     * @param value char to print
     * @returns printed char
     *
     */
    int _putc(int value);
 
    /** draw a character on given position out of the active font to the LCD
     *
     * @param x x-position of char (top left)
     * @param y y-position
     * @param c char to print
     *
     */
    void character(int x, int y, int c);
 
    /** setup cursor position
     *
     * @param x x-position (top left)
     * @param y y-position
     */
    void locate(int x, int y);
    
    /** setup auto update of screen 
      *
      * @param up 1 = on , 0 = off
      * if switched off the program has to call copy_to_lcd() 
      * to update screen from framebuffer
      */
    void set_auto_up(unsigned int up);
 
    /** get status of the auto update function
      *
      *  @returns if auto update is on
      */
    unsigned int get_auto_up(void);
		
		/** select the font to use
      *
      * @param f pointer to font array
      *
      *   font array can created with GLCD Font Creator from http://www.mikroe.com
      *   you have to add 4 parameter at the beginning of the font array to use:
      *   - the number of byte / char
      *   - the vertial size in pixel
      *   - the horizontal size in pixel
      *   - the number of byte per vertical line
      *   you also have to change the array to char[]
      *
      */
    void set_font(unsigned char* f);
			
			/** draw a horizontal line
      *
      * @param x0 horizontal start
      * @param x1 horizontal stop
      * @param y vertical position
      * @param ,1 set pixel ,0 erase pixel
      *
      */
    void hline(int x0, int x1, int y, int colour);
 
    /** draw a vertical line
     *
     * @param x horizontal position
     * @param y0 vertical start
     * @param y1 vertical stop
     * @param ,1 set pixel ,0 erase pixel
     */
    void vline(int y0, int y1, int x, int colour);
 
    /** Init the ST7567 LCD controller
     *
     */
    void lcd_begin(uint8_t SDI, uint8_t SCK, uint8_t CS, uint8_t _DC, uint8_t _RST);
 
    /** Write data to the LCD controller
     *
     * @param dat data written to LCD controller
     *
     */
    void sendData(unsigned char value);
 
    /** Write a command the LCD controller
      *
      * @param cmd: command to be written
      *
      */
    void sendCmd(unsigned char value);
 
    void wr_cnt(unsigned char cmd);
		
		void displayMainInterface(uint8_t edit_section, uint8_t hr, uint8_t mins, uint8_t date, 
															uint8_t month, uint8_t year, uint8_t product, float weight, uint8_t conn);