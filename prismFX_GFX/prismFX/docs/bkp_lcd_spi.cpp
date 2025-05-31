#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "kidbright32.h"
#include "lcd_spi.h"
#include "font_5x7.h"
#include "font_9x15.h"

// 26 is out OUT1, 27 is OUT2, 0 is default
#define CS_PIN	GPIO_NUM_0
const uint16_t BUFFER_SIZE = 32;	// max number of bytes that can sent with spi->write

// INITIALIZATION OF properties

LCD_SPI::LCD_SPI(int dev_addr) {
	channel = 0;
	address = dev_addr;
	printf("new LCD_SPI object %p\n", this);
}

void LCD_SPI::init(void) {
	printf("init %02x %p\n", address, this);
	flag = 0;
	set_flag = 0;
	clr_flag = 0;
	state = s_detect;
	cmdCount = cmdPut = cmdPull = 0;
	mcpPort = MCP23S17_REG_GPIOA;		// varies by display if multiple displays
	acmd = 0xcf;//0xe7;						// which IOs are connected to the display
	adat = 0xdf;//0xef;
	arst = 0xf7;//0xfb;
//	acmd = 0x3F;						// 2nd display
//	adat = 0x7f;
//	arst = 0xBF;

	backC = 0;		// black
	foreC = 0xffff; // white
	colS = rowS = colM = rowM = display_rotation = 0;
		
// these are used to switch GPIO0 to an input or output, switch to input to disable it		
	io_conf_0in.intr_type = GPIO_INTR_DISABLE; // disable interrupt
	io_conf_0in.mode = GPIO_MODE_INPUT; // set as input mode
	io_conf_0in.pin_bit_mask = 1ULL; // pin 0 bit mask for GPIO0
	io_conf_0in.pull_down_en = GPIO_PULLDOWN_DISABLE; // disable pull-down mode
	io_conf_0in.pull_up_en = GPIO_PULLUP_ENABLE; // disable pull-up mode

	io_conf_0out.intr_type = GPIO_INTR_DISABLE; // disable interrupt
	io_conf_0out.mode = GPIO_MODE_OUTPUT; // set as output mode
	io_conf_0out.pin_bit_mask = 1ULL; // pin 0 bit mask for GPIO0
	io_conf_0out.pull_down_en = GPIO_PULLDOWN_DISABLE; // disable pull-down mode
	io_conf_0out.pull_up_en = GPIO_PULLUP_DISABLE; // disable pull-up mode

	initialized = true;
}

// HARDWARE OUTPUT FUNCTIONS (There are no hardware input functions)

esp_err_t LCD_SPI::wrMCP(uint8_t reg, uint8_t val){		// configures the MCP23S17 output for the St7789 RST, DAT, & CS pins
	esp_err_t rc;
	uint8_t data[3];
	data[0] = address << 1;
	data[1] = reg;
	data[2] = val;
	CMD_SET = val == acmd;
/*
The KB software configures pin 0 as the CS pin for SPI. 
CS is asserted whether we are accessing the MCP23S17 GPIO chip or the ST7789 Display
Having both chips selected at the same time is very bad. It won't work.
Solutions
1.	Use out1 or out2, pins 26 or 27, for CS and leave pin 0 unconnected.
	This works well but may break user projects that are using the selected output pin.
2.	Change pin 0 to an input when we want to access the ST7789 Display.
	Seems to work but needs more testing with other SPI devices.

Neither of these solutions is faster than the other. drawPixel takes about same time either way.
*/
#if CS_PIN == 0
	gpio_config(&io_conf_0out);	// change pin 0 an output
#endif
	gpio_set_level(CS_PIN, CS_PIN==26 || CS_PIN==27);	// invert for OUT1 (26) and OUT2 (27)
	rc = spi->write(channel, address, data, 3);
// the MCP23S017 PAx pins change after 24us, but spi-write returns after 80 us ???
	gpio_set_level(CS_PIN, CS_PIN!=26 && CS_PIN!=27);
#if CS_PIN == 0
	if(val != 0xff)		// if true then there is no intention of writing to the ST7789 display
		gpio_config(&io_conf_0in);	// change pin 0 to an input fo ST7789 writes
#endif
	return rc;
}

void LCD_SPI::writecommand(uint8_t cmd){	// sets up ST7789 pins using the MCP23S17, then send command to ST7789
	if(!CMD_SET)	wrMCP(mcpPort, acmd);
	spi->write(channel, address, &cmd, 1);
}

void LCD_SPI::writedata(uint8_t data){	// sets up ST7789 pins using the MCP23S17, then 1 byte of data to ST7789
	if( CMD_SET)	wrMCP(mcpPort, adat);
 	spi->write(channel, address, &data, 1);
}

void LCD_SPI::sendCnD(uint8_t cmd, uint8_t * data, uint16_t dCnt){
	if(!CMD_SET)	wrMCP(mcpPort, acmd);	// DC lo, CS lo
	spi->write(channel, address, &cmd, 1);	// cmd
	if( CMD_SET)	wrMCP(mcpPort, adat);	// DC hi, CS lo
	spi->write(channel, address, data, dCnt);// data
}

// sets up ST7789 to receive bulk data, 2 * (1+x1-x0) * (1+y1-y0) bytes at specific coordinates
// working to optimize this function as it is called often, 1x/point, 1x/character and the hardware is slow
void LCD_SPI::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
	if(x1 > x2){
		uint16_t temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if(y1 > y2){
		uint16_t temp = y1;
		y1 = y2;
		y2 = temp;
	}
	if(display_rotation == 3){	x1 += 80;	x2 += 80;	}	// pins on the right
	if(display_rotation == 2){	y1 += 80;	y2 += 80;	}	// pins on the bottom

	uint8_t buf[4], cmdCol = 0x2a, cmdRow = 0x2b, cmdColor = 0x2c;	// st7789 command bytes
	buf[0] = x1>>8; buf[1] = x1&255; buf[2] = x2>>8; buf[3] = x2&255;	// vertical bounds
	sendCnD(cmdCol, buf, 4);
	buf[0] = y1>>8; buf[1] = y1&255; buf[2] = y2>>8; buf[3] = y2&255;	// horizontal bounds
	sendCnD(cmdRow, buf, 4);
	if(!CMD_SET)	wrMCP(mcpPort, acmd);	// DC lo, CS lo
	spi->write(channel, address, &cmdColor, 1);	// now prepared to receive color data
}

void LCD_SPI::bufferOut(uint8_t *bufr, uint16_t sz){	// for sending bulk data in BUFFER_SIZE chunks
	if( CMD_SET)	wrMCP(mcpPort, adat);  		// DC hi, CS lo
	while(sz > BUFFER_SIZE){
		spi->write(channel, address, bufr, BUFFER_SIZE);
		bufr += BUFFER_SIZE;
		sz -= BUFFER_SIZE;
	}
	if(sz)						// remainder of buffer, if any
		spi->write(channel, address, bufr, sz);
	wrMCP(mcpPort, 0xff);      	// rst, dc, & cs all high
}

// Geometric Functions: Point, Line, Circle, Rectange, Bitmap, maybe triangle?

void LCD_SPI::drawPixel(uint16_t x, uint16_t y, uint16_t color){
	if(x > _W || y > _H)  return;		// when cropping a single pixel, the result is a pixel or no pixel
	setAddrWindow(x, y, x, y);
	uint8_t buf[2];	buf[0] = color>>8;	buf[1] = color & 255;
	bufferOut(buf, sizeof(buf));
}

void LCD_SPI::drawLineH(uint16_t x, uint16_t y, uint16_t w, uint16_t color){
	uint8_t buf[w*2], *p=buf;
	for(int i=0; i<w; i++){
		*p++ = color >> 8;
		*p++ = color & 255;
	}
	setAddrWindow(x, y, x+w, y);
	bufferOut( buf, sizeof(buf));
}

void LCD_SPI::drawLineV(uint16_t x, uint16_t y, uint16_t h, uint16_t color){
	uint8_t buf[h*2], *p=buf;	// max size will be 240x2
	for(int i=0; i<h; i++){
		*p++ = color >> 8;
		*p++ = color & 255;
	}
	setAddrWindow(x, y, x, y+h);
	bufferOut( buf, sizeof(buf));
}

void LCD_SPI::drawLine(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1, uint16_t color){
	if(x == x1)	drawLineV(x, y, y1-y+1, color);		// draw FAST vertical line
	if(y == y1)	drawLineH(x, y, x1-x+1, color);		// draw FAST horizontal line
// TODO draw an oblique line}
}

// TODO drawCircle
// TODO drawBitmap
// TODO drawTriangle, maybe?? code exists from buydisplay.com but how useful is it?

void LCD_SPI::drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, bool fill){
	if(fill){
		uint8_t bufr[BUFFER_SIZE], *p = bufr;		// max number of bytes that can be sent over SPI bus
		uint16_t i=(1+x2-x1)*(1+y2-y1);
		setAddrWindow(x1, y1, x2, y2);
		wrMCP(mcpPort, adat);  // dc high, cs active (low)
		while(i--){
			*p++ = color >> 8;
			*p++ = color & 255;
			if(p-bufr == BUFFER_SIZE){	// size = difference of two pointers
				spi->write(channel, address, bufr, BUFFER_SIZE);
				p = bufr;
			}
		}
		if(p-bufr)	spi->write(channel, address, bufr, p-bufr);
		wrMCP(mcpPort, 0xff);        // rst, dc, & cs all high
	}else{
		drawLineH(x1,y1,1+x2-x1,color);
		drawLineH(x1,y2,1+x2-x1,color);
		drawLineV(x1,y1,1+y2-y1,color);
		drawLineV(x2,y1,1+y2-y1,color);
	}
}

// Character and string functions

void LCD_SPI::drawChar5x7(char character){
	uint8_t buf[96], *p = buf;
	uint16_t i, j, k, c, index = (character & 0xff)*5;

	if(foreC == backC){			// transparent printing. Just write pixels over the background
		if( character != '\n'){
			for(i=0; i<5; i++){
				k = font_5x7[index + i];
				for(j=0; j<8; j++)
					if(k & (1<<j))
						drawPixel(colS*6+i, rowS*8+j, foreC);
			}
			colS++;				// advance column position
			if(colS<40)	return;	// return if not wrapping
		}
	}else{						// fill background and foreground
		if(character == '\n'){	// if a newline
			for( i=0; i<8; i++)	// clear to the end of the line
				drawLineH(colS*6, rowS*8+i, 240-colS*6, backC);
		}else{					// printable character
			for(j=0; j<8; j++){								// for each line
				for(i=0; i<6; i++){							// for each column
					k = i<5 ? font_5x7[index + i] : 0;		// last column is inter character gap, all bits off
					c = (k & (1<<j)) ? foreC : backC;		// if bit set then use foreground color, else background
					*p++ = c >> 8;							// put appropriate color in buffer
					*p++ = c & 255;
				}
			}
			setAddrWindow(colS*6, rowS*8, colS*6+5, rowS*8+7);	
			bufferOut(buf, sizeof(buf));					// write the character to the display
			colS++;				// advance column position
			if(colS<40)	return;	// return if not wrapping
		}
	}
	colS = 0;	// wrap to next line
	rowS ++;
	rowS %= 30;	// wrap to top of display if passed the end
}

void LCD_SPI::drawStr5x7(char *message){
	char *p = message;
	while(*p)	drawChar5x7(*p++);
}

void LCD_SPI::drawChar9x14(char character){
	uint8_t	buf[15][10][2], chr = (character & 0x7f) - 0x20;	// trim to only printable ascii characters
	uint16_t i, j, k, c, index = chr * 9 / 8;
	if(foreC == backC){				// transparent printing. Just write pixels over the background
		if(character > ' '){		// printable character?
			for(i=0; i<14; i++){
				k = (font_9x15[index + i*107]) | (font_9x15[index + 1 + i*107] << 8);
				k >>= chr & 7;
				for(j=0; j<9; j++)
					if(k & 1<<j)
						drawPixel(colM*10+j, rowM*15+i, foreC);
			}
		}else if(character == '\n')		// newline character?
			colM = 99;					// force newline
		colM++;
		if( colM < 24 )	return;
	}else{
		if(character == '\n'){			// if a newline character
			for(int  i=0; i<15; i++)	// clear to the end of the line
				drawLineH(colM*10, rowM*15+i, 240-colS*10, backC);
		}else{							// printable character - not transparent
			for(j=0; j<10; j++){		// fill inter line gap
				buf[14][j][0] = backC >> 8;
				buf[14][j][1] = backC & 255;
			}
			// The font file is a bit array, 9 bits/char packed together, 95 chars/107 bytes
			for(i=0; i<14; i++){		// for each line
				k = (font_9x15[index + i*107]) | (font_9x15[index + 1 + i*107] << 8);
				k >>= chr & 7;
				for(j=0; j<9; j++){		// for each column
					c = (k & (1<<j)) ? foreC : backC;
					buf[i][j][0] = c >> 8;
					buf[i][j][1] = c & 255;
				}
				buf[i][9][0] = backC >> 8;	// fill inter character gap
				buf[i][9][1] = backC & 255;
			}
			setAddrWindow(colM*10, rowM*15, colM*10+9, rowM*15+14);
			bufferOut( (uint8_t *)buf, sizeof(buf) );
			colM++;
			if( colM < 24)	return;
		}
	}
	colM = 0;		// wrap to next line
	rowM++;
	rowM &= 15;		// 16 rows, 15 pixels high, wrap to first if past last
}

void LCD_SPI::drawStr9x14(char *message){
	char *p = message;
	while(*p)	drawChar9x14(*p++);
}

void LCD_SPI::drawPlotDetail(uint16_t pnt, uint8_t arg, bool redraw){
	uint16_t saveFC = foreC, saveBC = backC, saveCM = colM, saveRM = rowM;
	backC = C_BLACK;	// black background
	if(arg == 1){
		rowM = 11;	colM =  0;
		foreC = C_WHITE;	drawStr9x14("Var");
		foreC = C_RED  ;	drawStr9x14(pd[0].variable);	pd[0].color = C_RED;
		foreC = C_GREEN;	drawStr9x14(pd[1].variable);	pd[1].color = C_GREEN;
		foreC = C_BLUE ;	drawStr9x14(pd[2].variable);	pd[2].color = C_BLUE;

		rowM = 15;	colM =  0;
		foreC = C_WHITE;	drawStr9x14("Unt");
		foreC = C_RED  ;	drawStr9x14(pd[0].units);
		foreC = C_GREEN;	drawStr9x14(pd[1].units);
		foreC = C_BLUE ;	drawStr9x14(pd[2].units);
	}
	if(arg == 2){
		char tmp[8];
		int y;
		rowM = 12; colM = 0;
		foreC = C_WHITE;	drawStr9x14("Max");
		foreC = C_RED  ;	snprintf(tmp, 8, pd[0].format, pd[0].max);	drawStr9x14(tmp);
		foreC = C_GREEN;	snprintf(tmp, 8, pd[1].format, pd[1].max);	drawStr9x14(tmp);
		foreC = C_BLUE ;	snprintf(tmp, 8, pd[2].format, pd[2].max);	drawStr9x14(tmp);

		rowM = 13; colM = 0;
		foreC = C_WHITE;	drawStr9x14("Cur");
		foreC = C_RED  ;	snprintf(tmp, 8, pd[0].format, pd[0].data[pnt]);	drawStr9x14(tmp);
		foreC = C_GREEN;	snprintf(tmp, 8, pd[1].format, pd[1].data[pnt]);	drawStr9x14(tmp);
		foreC = C_BLUE ;	snprintf(tmp, 8, pd[2].format, pd[2].data[pnt]);	drawStr9x14(tmp);

		rowM = 14; colM = 0;
		foreC = C_WHITE;	drawStr9x14("Min");
		foreC = C_RED  ;	snprintf(tmp, 8, pd[0].format, pd[0].min);	drawStr9x14(tmp);
		foreC = C_GREEN;	snprintf(tmp, 8, pd[1].format, pd[1].min);	drawStr9x14(tmp);
		foreC = C_BLUE ;	snprintf(tmp, 8, pd[2].format, pd[2].min);	drawStr9x14(tmp);

		if(redraw){
			drawRect(0, 64, 239, 164, C_BLACK, true);		// clear graph
			for(int i=84; i<=144; i+=20)	drawLineH(0,i,240,0x2084);	// y grid
			for(int i=40; i<200; i+=40)		drawLineV(i,64,101,0x1042);	// x grid
			drawRect(0, 64, 239, 164, 0x4108, false);		// outine of plot area
			for(int j=0; j<3; j++){
				float denom = 100.0 / (pd[j].max - pd[j].min);
				for(int i=0; i<pdCnt; i++){
					y = (pd[j].data[i] - pd[j].min) * denom;
					drawPixel(i, 164-y, pd[j].color);
				}
			}
		}
		for(int j=0; j<3; j++){
			float denom = 100.0 / (pd[j].max - pd[j].min);
			y = (pd[j].data[pnt] - pd[j].min) * denom;
			drawPixel(pnt, 164-y, pd[j].color);
		}
		for(int i=pnt+1; i<pnt+8; i++)
			drawLineV(i%240, 64, 101, C_BLACK);
		drawLineV((pnt+4)%240, 64, 101, C_YELLOW);
	}




	foreC = saveFC; backC = saveBC; colM = saveCM; rowM = saveRM;
}

// Main loop, initialize and then look for blocks commands to process
// This runs in a different thread than the code that processes the block commands

void LCD_SPI::process(Driver *drv) {
// TODO review sequence of states
	esp_err_t rc;
	spi = (SPIDev *)drv;		// don't want to keep passing this to functions
	spi->sck_speed_set( SCK_SPEED_HIGH );	// 4MHz isn't very high
	uint8_t bufPORCTRL[] = {0x0c,0x0c,0x00,0x33,0x33};
	uint8_t bufE0[] = {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23};
	uint8_t bufE1[] = {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23};

	switch (state) {
		case s_detect:
			printf("s_detect-->");
			// detect spi device
			state = s_error;													// assume failure
			if (spi->detect(channel, address) == ESP_OK) {
				// init MCP23S17									making all but the 3 required pins inputs is safer???
				if( (rc = wrMCP(MCP23S17_REG_IOCONA, 8)) != ESP_OK)	return;		// EN_HA - enable hardware addressing
				if( (rc = wrMCP(MCP23S17_REG_IOCONB, 8)) != ESP_OK)	return;
				if( (rc = wrMCP(MCP23S17_REG_IODIRA, 0)) != ESP_OK)	return;		// all outputs - all pins are outputs
				if( (rc = wrMCP(MCP23S17_REG_IODIRB, 0)) != ESP_OK)	return;
				if( (rc = wrMCP(MCP23S17_REG_GPIOA , 0xff)) != ESP_OK)	return;	// all outputs high
				if( (rc = wrMCP(MCP23S17_REG_GPIOB , 0xff)) != ESP_OK)	return;	
				// reset the ST7789 display
				tickcnt = get_tickcnt();
				while (! is_tickcnt_elapsed(tickcnt, 5)) ;

				wrMCP(mcpPort, arst);
				tickcnt = get_tickcnt();
				while (! is_tickcnt_elapsed(tickcnt, 20)) ;

				wrMCP(mcpPort, 0xff);
				tickcnt = get_tickcnt();
				while (! is_tickcnt_elapsed(tickcnt, 150)) ;
				printf("s_cmd_init\n");

				// init ST7789

				writecommand(0x36); 
				writedata(0x00);
					
				writecommand(0x3A); // COLMOD
				writedata(0x05);

				sendCnD(0xB2, bufPORCTRL, sizeof(bufPORCTRL));

				writecommand(0xB7);   // GCTRL
				writedata(0x35);  

				writecommand(0xBB);
				writedata(0x19);

				writecommand(0xC0);
				writedata(0x2C);

				writecommand(0xC2);
				writedata(0x01);

				writecommand(0xC3);
				writedata(0x12);   

				writecommand(0xC4);
				writedata(0x20);  

				writecommand(0xC6); 
				writedata(0x0F);    

				writecommand(0xD0); 
				writedata(0xA4);
				writedata(0xA1);

				sendCnD(0xE0, bufE0, sizeof(bufE0));
				sendCnD(0xE0, bufE1, sizeof(bufE1));

				writecommand(0x21); 

				writecommand(0x11);    	//Exit Sleep
				wrMCP(mcpPort, 0xff);	// CS off

				tickcnt = get_tickcnt();
				while(!is_tickcnt_elapsed(tickcnt, 120));	//	delay(120);

				writecommand(0x29);    	//Display on
				wrMCP(mcpPort, 0xff);	// CS off

				state = s_cmd_init;					// MCP23S17 initialized
			}
			break;

		case s_cmd_init:
			state = s_idle;
			break;

		case s_idle:
// TODO problem, the cmds buffer can be overrun! needs to be a sync'ing mechanism!
			if( cmdCount > 0 ){				
				printf("command count = %d\n", cmdCount);
				blkCmd *cmd = &cmds[cmdPull];
				switch( cmd->code ){
					case cmd_clear:
						display_rotation = cmd->parm[0];
						drawRect(0,0,_W,_H,cmd->parm[1],true);	// fill entire screen with color in parm[0]
						writecommand(0x36);
						writedata(rot[display_rotation]);
						wrMCP(mcpPort, 0xff);	// CS off
						rowS = colS = rowM = colM = 0;	// reset cursors
						break;
					case cmd_setColor:
						foreC = cmd->parm[0];
						backC = cmd->parm[1];
						break;
					case cmd_printTiny:
						if(cmd->parm[0] > 0)	colS = cmd->parm[0]-1;
						if(cmd->parm[1] > 0)	rowS = cmd->parm[1]-1;
						drawStr5x7 (cmd->str);
						break;
					case cmd_print:
						if(cmd->parm[0] > 0)	colM = cmd->parm[0]-1;
						if(cmd->parm[1] > 0)	rowM = cmd->parm[1]-1;
						drawStr9x14(cmd->str);
						break;
					case cmd_point:
						drawPixel(cmd->parm[0], cmd->parm[1], cmd->parm[2]);
						break;
					case cmd_line:
						drawLine(cmd->parm[0], cmd->parm[1], cmd->parm[2], cmd->parm[3], cmd->parm[4]);
						break;
					case cmd_rect:
						drawRect(cmd->parm[0], cmd->parm[1], cmd->parm[2], cmd->parm[3], cmd->parm[4], cmd->flag);
						break;
					case cmd_plot:
						drawPlotDetail(cmd->parm[0], cmd->parm[1], cmd->flag);
						break;
				}
				cmdPull++;
				cmdPull %= MAX_CMD_BUF;
				cmdCount--;
			}
			break;

		case s_error:
			error = true;				// set error flag
			initialized = false;		// clear initialized flag
			tickcnt = get_tickcnt();	// get current tickcnt
			state = s_wait;				// goto wait and retry with detect state
			break;

		case s_wait:
			if (is_tickcnt_elapsed(tickcnt, 1000)) {	// delay 1000ms before retry detect
				state = s_detect;
			}
			break;
	}
}

// commands from BLOCKLY - cannot call functions defined above.
// Just store the command info in a circular buffer for the process function

void LCD_SPI::print    (uint8_t row, uint8_t col, char *message, uint8_t siz) {
	char *p = message;
	while(*p){		// the str member of the cmds structure is only 32 bytes - long strings must be split into smaller cmds
		char *q = cmds[cmdPut].str;
		cmds[cmdPut].code = siz == 0 ? cmd_printTiny : cmd_print;
		cmds[cmdPut].parm[0] = row;
		cmds[cmdPut].parm[1] = col;
		for(int i=1; i<sizeof(cmds[cmdPut].str) && *p; i++)
			*q++ = *p++;// copy the str, or part of it, into the cmd buffer
		*q = 0;			// terminate the c-str with a null chharacter
		cmdPut++;
		cmdPut %= MAX_CMD_BUF;
		cmdCount++;
		row = col = 0;	// ignore for subsequent commands - just continue from where the last print char ended
	}
	vTaskDelay(cmdCount);
}

void LCD_SPI::point(uint16_t x1, uint16_t y1, uint32_t color){
	cmds[cmdPut].code = cmd_point;
	cmds[cmdPut].parm[0] = x1;
	cmds[cmdPut].parm[1] = y1;
	cmds[cmdPut].parm[2] = color565(color);
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color){
	cmds[cmdPut].code = cmd_line;
	cmds[cmdPut].parm[0] = x1;
	cmds[cmdPut].parm[1] = y1;
	cmds[cmdPut].parm[2] = x2;
	cmds[cmdPut].parm[3] = y2;
	cmds[cmdPut].parm[4] = color565(color);
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color, bool fill){
	cmds[cmdPut].code = cmd_rect;
	cmds[cmdPut].flag = fill;
	cmds[cmdPut].parm[0] = x1;
	cmds[cmdPut].parm[1] = y1;
	cmds[cmdPut].parm[2] = x2;
	cmds[cmdPut].parm[3] = y2;
	cmds[cmdPut].parm[4] = color565(color);
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::clear(uint8_t rotation, uint32_t color) {
	cmds[cmdPut].code = cmd_clear;
	cmds[cmdPut].parm[0] = rotation & 3;
	cmds[cmdPut].parm[1] = color565(color);
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::setTextColor(uint32_t foreground, uint32_t background){
	cmds[cmdPut].code = cmd_setColor;
	cmds[cmdPut].parm[0] = color565(foreground);
	cmds[cmdPut].parm[1] = color565(background);
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::triangle  (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3	, uint32_t color, bool fill ){
// stub for blockly testing
}

void LCD_SPI::circle  	(uint16_t x1, uint16_t y1, uint16_t r											, uint32_t color, bool fill ){
// stub for blockly testing
}

void LCD_SPI::image  	(uint16_t x1, uint16_t y1, uint16_t imageID, double scale){
// stub for blockly testing
}

char * LCD_SPI::num2str(double val, uint8_t wid, uint8_t dig, uint8_t fmt){
	static char buf[40], *p; // buffer for results
	if( wid > sizeof(buf) - 1)	wid = sizeof(buf) - 1;
	switch(fmt){
		case 0:	// integer
			sprintf(buf, "%*ld", wid, (long)val);
			break;
		case 1: // hex
			sprintf(buf, "%*lX", wid, (long)val);
			break;
		case 2: // hex with leading 0
			sprintf(buf, "%0*lX", wid, (long)val);
			break;
		case 3: // fixed point
			sprintf(buf, "%*.*f", wid, dig, val);
			break;
		case 4: // exponential
			sprintf(buf, "%*.*E", wid, dig, val);
			break;
		default:
			p = buf;
			while(wid--)	*p++ = '?';
			*p = 0;
	}
	return buf;
}

uint16_t LCD_SPI::color565(uint32_t rgb) {	// convert 24 bit RGB888 to 16 bit RGB565
  return ((rgb >> 8) & 0xF800) | ((rgb >> 3) & 0x7e0) | ((rgb >> 3) & 0x1f);
}

// Unused but seemingly required functions

int  LCD_SPI::prop_count(void) 						{	return false;}	// not supported
bool LCD_SPI::prop_name (int index, char *name  ) 	{	return false;}	// not supported
bool LCD_SPI::prop_unit (int index, char *unit  ) 	{	return false;}	// not supported
bool LCD_SPI::prop_attr (int index, char *attr  ) 	{	return false;}	// not supported
bool LCD_SPI::prop_read (int index, char *value ) 	{	return false;}	// not supported
bool LCD_SPI::prop_write(int index, char *value ) 	{	return false;}	// not supported

void LCD_SPI::initPlot(uint8_t index, char * variable, char * units, char decimals){
	if(index > 2)	return;		// cannot do this
	memset(&pd[index], 0, sizeof(plotData));
	char tmp[8];

	strncpy(tmp, variable, 7);
	tmp[7] = 0;		// null terminator
	while(strlen(tmp) < 7){
		for(int i=7; i; i--)
			tmp[i] = tmp[i-1];					// shift 1 character
		*tmp = ' ';
	}
	strcpy(pd[index].variable, tmp);

	strncpy(tmp, units, 7);
	tmp[7] = 0;		// null terminator
	while(strlen(tmp) < 7){
		for(int i=7; i; i--)
			tmp[i] = tmp[i-1];					// shift 1 character
		*tmp = ' ';
	}
	strcpy(pd[index].units, tmp);

	sprintf(pd[index].format,"%%7.%df", (int8_t)decimals);

	cmds[cmdPut].code = cmd_plot;
	cmds[cmdPut].parm[0] = 0;
	cmds[cmdPut].parm[1] = 1;
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);
}

void LCD_SPI::plotPoint(double r, double g, double b){
	float min, max, oldmin[3], oldmax[3];
	for(int i=0; i<3; i++){
		oldmin[i] = pd[i].min;
		oldmax[i] = pd[i].max;
	}

	pd[0].data[pdPos] = r;
	pd[1].data[pdPos] = g;
	pd[2].data[pdPos] = b;
	pdCnt++;
	if(pdCnt > 240)	pdCnt = 240;
	for(int j=0; j<3; j++){
		min = 1e20;
		max = -1e20;
		for(int i=0; i<pdCnt; i++){
			if(pd[j].data[i] < min)	min = pd[j].data[i];
			if(pd[j].data[i] > max)	max = pd[j].data[i];
		}
		pd[j].max = max;
		pd[j].min = min;
	}
	// plot it
	cmds[cmdPut].flag = false;		// assume no change in any of the mins or maxes
	for(int i=0; i<3; i++){			// we will need to redraw the entire plot area if a min or max changed
		cmds[cmdPut].flag |= pd[i].min != oldmin[i];
		cmds[cmdPut].flag |= pd[i].max != oldmax[i];
	}
	cmds[cmdPut].code = cmd_plot;
	cmds[cmdPut].parm[0] = pdPos;
	cmds[cmdPut].parm[1] = 2;
	cmdPut++;
	cmdPut %= MAX_CMD_BUF;
	cmdCount++;
	vTaskDelay(cmdCount);

	pdPos++;
	pdPos %= 240;
}
