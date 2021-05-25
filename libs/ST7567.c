#include "ST7567.h"

//ToDO:Check why orientation char X and draw mode cannot be effectively initialized here instead of in ST7567.h file

/**
 * @brief SPI user event handler.
 * @param event
 */
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
    //NRF_LOG_INFO("Transfer completed.");
    if (m_rx_buf[0] != 0)
    {
        NRF_LOG_INFO(" Received:");
        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
    }
}
/**
 * @brief initialize GPIOs
 * @param 
 */
static void gpioInit()
{
	nrf_gpio_cfg_output(DC_PIN);
  nrf_gpio_cfg_output(RST_PIN);
}

static void sendSPI(uint8_t val)
{
    spi_xfer_done = false;	
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, &val, 1, m_rx_buf, 0));
		while(!spi_xfer_done)
		{
				//NRF_LOG_INFO("wait.");
		}
}

/**
 * @brief send command to LCD.
 * @param 
 */
void sendCmd(uint8_t cmd)
{
		nrf_gpio_pin_clear(DC_PIN); //configure for command transmission
	  //NRF_LOG_INFO("Set DC pin low");
	  //NRF_LOG_FLUSH();
		sendSPI(cmd);
}

/**
 * @brief send data to LCD.
 * @param 
 */
void sendData(uint8_t data)
{ 
		nrf_gpio_pin_set(DC_PIN); //configure for data transmission
	  //NRF_LOG_INFO("Set DC pin HIGH");
	  //NRF_LOG_FLUSH();
		sendSPI(data);
}

// update lcd 
void copy_to_lcd(void)
{
    for (int page = 0; page < 8; page++) {
        sendCmd(0x00);        // set column low nibble 0
        sendCmd(0x10);        // set column hi  nibble 0
        sendCmd(0xB0|page);   // set page address  0
        nrf_gpio_pin_set(DC_PIN);
 
        for(int i=page*128; i<(page*128+128); i++) {
            sendData(buffer[i]);
        }
    }
}

// set cursor position
void locate(int x, int y)
{
    char_x = x;
    char_y = y;
}

// set font
void set_font(unsigned char* f)
{
    font = f;
}

void lcd_begin(uint8_t _SDI, uint8_t _SCK, uint8_t _CS, uint8_t _DC, uint8_t _RST)
{
    DC_PIN = _DC;
		RST_PIN = _RST;
		CS_PIN = _CS;
		SDI_PIN = _SDI;
	  SCK_PIN = _SCK;
		
		gpioInit();
	
		nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = CS_PIN; //SPI_SS_PIN;
    spi_config.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config.mosi_pin = SDI_PIN; //SPI_MOSI_PIN;
    spi_config.sck_pin  = SCK_PIN;//SPI_SCK_PIN;
		spi_config.frequency    = NRF_DRV_SPI_FREQ_8M;                     
    spi_config.mode         = NRF_DRV_SPI_MODE_0;                      
    spi_config.bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST; 
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    
		nrf_gpio_pin_clear(DC_PIN);
    nrf_gpio_pin_set(SPI_SS_PIN);
    nrf_gpio_pin_clear(RST_PIN);                      // display reset
    nrf_delay_ms(50);
    nrf_gpio_pin_set(RST_PIN);                        // end reset
    nrf_delay_ms(5);
 
    /* Start Initial Sequence ----------------------------------------------------*/
 
    sendCmd(0xE2);            // S/W RESWT
    sendCmd(0xA3);            // LCD bias
    sendCmd(0xAF);            // Display ON
    sendCmd(0xA0);            // segment direction.
    sendCmd(0xC8);            // Common Direction.
    sendCmd(0x22);            // Regultion resistor select  //25
 
    sendCmd(0x81);            // EV Select.
    sendCmd(0x3f);            // Select EV value.
 
    sendCmd(0x2f);            // Power control
 
    sendCmd(0x40);            // Initial display line 40
    sendCmd(0xB0);            // Set page address
    sendCmd(0x10);            // Set coloumn addr  MSB
    sendCmd(0x00);            // Set coloumn addr LSB
    sendCmd(0xAF);            // Display ON
    sendCmd(0xA4);            // A5 .Normal display, all pixels OFF.
    sendCmd(0xA6);            // A7 .Normal display (Inverse Pixel)
 
    // clear and update LCD
    memset(buffer, 0x00, LCD_FB_SIZE);  // clear display buffer
    copy_to_lcd();
    auto_up = 1;              // switch on auto update
 
    locate(0,0);
    set_font((unsigned char*)Small_7);  // standart font
}

void setmode(int mode)
{
    draw_mode = mode;
}

// set one pixel in buffer
 
void pixel(int x, int y, int color)
{
    // first check parameter
    if(x > MAX_PIXEL_X || y > MAX_PIXEL_Y || x < 0 || y < 0) return;
 
    if(draw_mode == LCD_NORMAL) {
        if(color == 0)
            buffer[x + ((y/8) * 128)] &= ~(1 << (y%8));  // erase pixel
        else
            buffer[x + ((y/8) * 128)] |= (1 << (y%8));   // set pixel
    } else { // XOR mode
        if(color == 1)
            buffer[x + ((y/8) * 128)] ^= (1 << (y%8));   // xor pixel
    }
}

int width()
{
    if (orientation == 0 || orientation == 2)
        return MAX_PIXEL_Y;
    else
        return MAX_PIXEL_X;
}
 
int height()
{
    if (orientation == 0 || orientation == 2)
        return MAX_PIXEL_X;
    else
        return MAX_PIXEL_Y;
}

void character(int x, int y, int c)
{
    unsigned int hor,vert,offset,bpl,j,i,b;
    unsigned char* zeichen;
    unsigned char z,w;
 
    if ((c < 31) || (c > 127)) return;   // test char range
 
    // read font parameter from start of array
    offset = font[0];                    // bytes / char
    hor = font[1];                       // get hor size of font
    vert = font[2];                      // get vert size of font
    bpl = font[3];                       // bytes per line
 
    if (char_x + hor > width()) {
        char_x = 0;
        char_y = char_y + vert;
        if (char_y >= height() - font[2]) {
            char_y = 0;
        }
    }
 
    zeichen = &font[((c - ' ') * offset) + 4]; // start of char bitmap
    w = zeichen[0];                            // width of actual char
    // construct the char into the buffer
    for (j=0; j<vert; j++) {  //  vert line
        for (i=0; i<hor; i++) {   //  horz line
            z =  zeichen[bpl * i + ((j & 0xF8) >> 3)+1];
            b = 1 << (j & 0x07);
            if (( z & b ) == 0x00) {
                pixel(x+i,y+j,0);
            } else {
                pixel(x+i,y+j,1);
            }
 
        }
    }
 
    char_x += w;
}

int _putc(int value)
{
    if (value == '\n') {    // new line
        char_x = 0;
        char_y = char_y + font[2];
        if (char_y >= height() - font[2]) {
            char_y = 0;
        }
    } else {
        character(char_x, char_y, value);
        if(auto_up) copy_to_lcd();
    }
    return value;
}

void set_contrast(unsigned int o)
{
    contrast = o;
    sendCmd(0x81);      //  set volume
    sendCmd(o & 0x3F);
}

void line(int x0, int y0, int x1, int y1, int color)
{
    int   dx = 0, dy = 0;
    int   dx_sym = 0, dy_sym = 0;
    int   dx_x2 = 0, dy_x2 = 0;
    int   di = 0;
 
    dx = x1-x0;
    dy = y1-y0;
 
    //  if (dx == 0) {        /* vertical line */
    //      if (y1 > y0) vline(x0,y0,y1,color);
    //      else vline(x0,y1,y0,color);
    //      return;
    //  }
 
    if (dx > 0) {
        dx_sym = 1;
    } else {
        dx_sym = -1;
    }
    //  if (dy == 0) {        /* horizontal line */
    //      if (x1 > x0) hline(x0,x1,y0,color);
    //      else  hline(x1,x0,y0,color);
    //      return;
    //  }
 
    if (dy > 0) {
        dy_sym = 1;
    } else {
        dy_sym = -1;
    }
 
    dx = dx_sym*dx;
    dy = dy_sym*dy;
 
    dx_x2 = dx*2;
    dy_x2 = dy*2;
 
    if (dx >= dy) {
        di = dy_x2 - dx;
        while (x0 != x1) {
 
            pixel(x0, y0, color);
            x0 += dx_sym;
            if (di<0) {
                di += dy_x2;
            } else {
                di += dy_x2 - dx_x2;
                y0 += dy_sym;
            }
        }
        pixel(x0, y0, color);
    } else {
        di = dx_x2 - dy;
        while (y0 != y1) {
            pixel(x0, y0, color);
            y0 += dy_sym;
            if (di < 0) {
                di += dx_x2;
            } else {
                di += dx_x2 - dy_x2;
                x0 += dx_sym;
            }
        }
        pixel(x0, y0, color);
    }
    if(auto_up) copy_to_lcd();
}
 
void rect(int x0, int y0, int x1, int y1, int color)
{
 
    if (x1 > x0) line(x0,y0,x1,y0,color);
    else  line(x1,y0,x0,y0,color);
 
    if (y1 > y0) line(x0,y0,x0,y1,color);
    else line(x0,y1,x0,y0,color);
 
    if (x1 > x0) line(x0,y1,x1,y1,color);
    else  line(x1,y1,x0,y1,color);
 
    if (y1 > y0) line(x1,y0,x1,y1,color);
    else line(x1,y1,x1,y0,color);
 
    if(auto_up) copy_to_lcd();
}
 
void fillrect(int x0, int y0, int x1, int y1, int color)
{
    int l,c,i;
    if(x0 > x1) {
        i = x0;
        x0 = x1;
        x1 = i;
    }
 
    if(y0 > y1) {
        i = y0;
        y0 = y1;
        y1 = i;
    }
 
    for(l = x0; l<= x1; l ++) {
        for(c = y0; c<= y1; c++) {
            pixel(l,c,color);
        }
    }
    if(auto_up) copy_to_lcd();
}
 
 
 
void circle(int x0, int y0, int r, int color)
{
 
    int draw_x0, draw_y0;
    int draw_x1, draw_y1;
    int draw_x2, draw_y2;
    int draw_x3, draw_y3;
    int draw_x4, draw_y4;
    int draw_x5, draw_y5;
    int draw_x6, draw_y6;
    int draw_x7, draw_y7;
    int xx, yy;
    int di;
    //WindowMax();
    if (r == 0) {       /* no radius */
        return;
    }
 
    draw_x0 = draw_x1 = x0;
    draw_y0 = draw_y1 = y0 + r;
    if (draw_y0 < height()) {
        pixel(draw_x0, draw_y0, color);     /* 90 degree */
    }
 
    draw_x2 = draw_x3 = x0;
    draw_y2 = draw_y3 = y0 - r;
    if (draw_y2 >= 0) {
        pixel(draw_x2, draw_y2, color);    /* 270 degree */
    }
 
    draw_x4 = draw_x6 = x0 + r;
    draw_y4 = draw_y6 = y0;
    if (draw_x4 < width()) {
        pixel(draw_x4, draw_y4, color);     /* 0 degree */
    }
 
    draw_x5 = draw_x7 = x0 - r;
    draw_y5 = draw_y7 = y0;
    if (draw_x5>=0) {
        pixel(draw_x5, draw_y5, color);     /* 180 degree */
    }
 
    if (r == 1) {
        return;
    }
 
    di = 3 - 2*r;
    xx = 0;
    yy = r;
    while (xx < yy) {
 
        if (di < 0) {
            di += 4*xx + 6;
        } else {
            di += 4*(xx - yy) + 10;
            yy--;
            draw_y0--;
            draw_y1--;
            draw_y2++;
            draw_y3++;
            draw_x4--;
            draw_x5++;
            draw_x6--;
            draw_x7++;
        }
        xx++;
        draw_x0++;
        draw_x1--;
        draw_x2++;
        draw_x3--;
        draw_y4++;
        draw_y5++;
        draw_y6--;
        draw_y7--;
 
        if ( (draw_x0 <= width()) && (draw_y0>=0) ) {
            pixel(draw_x0, draw_y0, color);
        }
 
        if ( (draw_x1 >= 0) && (draw_y1 >= 0) ) {
            pixel(draw_x1, draw_y1, color);
        }
 
        if ( (draw_x2 <= width()) && (draw_y2 <= height()) ) {
            pixel(draw_x2, draw_y2, color);
        }
 
        if ( (draw_x3 >=0 ) && (draw_y3 <= height()) ) {
            pixel(draw_x3, draw_y3, color);
        }
 
        if ( (draw_x4 <= width()) && (draw_y4 >= 0) ) {
            pixel(draw_x4, draw_y4, color);
        }
 
        if ( (draw_x5 >= 0) && (draw_y5 >= 0) ) {
            pixel(draw_x5, draw_y5, color);
        }
        if ( (draw_x6 <=width()) && (draw_y6 <= height()) ) {
            pixel(draw_x6, draw_y6, color);
        }
        if ( (draw_x7 >= 0) && (draw_y7 <= height()) ) {
            pixel(draw_x7, draw_y7, color);
        }
    }
    if(auto_up) copy_to_lcd();
}
 
void fillcircle(int x, int y, int r, int color)
{
    int i,up;
    up = auto_up;
    auto_up = 0;   // off
    for (i = 0; i <= r; i++)
        circle(x,y,i,color);
    auto_up = up;
    if(auto_up) copy_to_lcd();
}
 

int columns()
{
    return width() / font[1];
}
 
 
 
int rows()
{
    return height() / font[2];
}


void displayMainInterface(uint8_t edit_section, uint8_t hours, uint8_t mins, uint8_t date,
													uint8_t month, uint8_t year, uint8_t product, float weight, uint8_t conn)
{
		static char date_buf[16] = "";
		static char product_buf[7] = "";
		static char weight_buf[9] = "";
		static char conn_buf[5] = "";
		
		//edit the date
		if(edit_section == 1 || edit_section == 0)
		{
				line(0, 20, 127, 20, 1);
				
				// Date
				locate(MAX_PIXEL_X - ((sizeof(date_buf))*5), 5);
				
				sprintf(date_buf,"%d:%d %d/%d/20%d", hours, mins,date, month, year);
			
				for(uint8_t i = 0; i < sizeof(date_buf); i++)
				{
						_putc(date_buf[i]);
				}
		}
		
		// edit the product
		if(edit_section == 2 || edit_section == 0)
		{
				// Product
				locate(0, 35);
				
				switch(product)
				{
					case 1: 
						sprintf(product_buf,"%d Maize", product);
						break;
					case 2: 
						sprintf(product_buf,"%d Beans", product);
						break;
					case 3: 
						sprintf(product_buf,"%d Peas", product);
						break;
					default:
						break;
				}
				
				for(uint8_t i = 0; i < sizeof(product_buf); i++)
				{
						_putc(product_buf[i]);
				}
		}
		
		// edit the weight
		if(edit_section == 3 || edit_section == 0)
		{
				line((sizeof(product_buf)+2)*5, 20, (sizeof(product_buf)+2)*5, 67, 1);
				
				// Weight
				sprintf(weight_buf,"%.2f KG", weight);
				
				locate((sizeof(product_buf)+3)*5, 35);
				
				for(uint8_t i = 0; i < sizeof(weight_buf); i++)
				{
						_putc(weight_buf[i]);
				}
		}
		
		//edit the connection status
		if(edit_section == 4 || edit_section == 0)
		{
				locate(0, 5);
				switch(conn)
				{
					case 0: 
						sprintf(conn_buf,"--X--");
						break;
					case 1: 
						sprintf(conn_buf,"CONN-");
						break;
				}
				for(uint8_t i = 0; i < sizeof(conn_buf); i++)
				{
						_putc(conn_buf[i]);
				}
		}
			
		
		copy_to_lcd();
		NRF_LOG_INFO("LCD started.");
		NRF_LOG_FLUSH();
}