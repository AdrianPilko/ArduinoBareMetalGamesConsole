// PAL video output using atmega328p.
// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312
#define FIRST_LINE_DRAWN 42
#define LAST_LINE_DRAWN 262
#define HALF_SCREEN 110


#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

#define COMMA 5
#define EXCLAMATION 6
#define SPACE 4

uint8_t alphafonts[26][8];
uint8_t symbols[7][8];
uint8_t screenMemory[YSIZE][XSIZE];

inline void putCharXY(uint8_t x,uint8_t y, uint8_t character)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		screenMemory[y+i][x] = alphafonts[character][i];
	}
}

inline void putSymXY(uint8_t x,uint8_t y, uint8_t symbolNumber)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		screenMemory[y+i][x] = symbols[symbolNumber][i];
	}
}

inline uint8_t convertToMyCharSet(char charToConvert)
{
	uint8_t rv = (uint8_t)charToConvert - 65;
	if ((rv > 25) || (rv < 0)) rv = 4;
	return rv;
}

void clearScreen()
{
	memset (&screenMemory[0][0],0,sizeof(uint8_t) * YSIZE * XSIZE );
}

int main()
{

	uint8_t printScreen = 0;

	symbols[0][0] = 0b00000000;
	symbols[0][1] = 0b01111110;
	symbols[0][2] = 0b01000010;
	symbols[0][3] = 0b01000010;
	symbols[0][4] = 0b01000010;
	symbols[0][5] = 0b01000010;
	symbols[0][6] = 0b01111110;
	symbols[0][7] = 0b00000000;

	symbols[1][0] = 0b11111111;
	symbols[1][1] = 0b10000001;
	symbols[1][2] = 0b10111101;
	symbols[1][3] = 0b10100101;
	symbols[1][4] = 0b10100101;
	symbols[1][5] = 0b10111101;
	symbols[1][6] = 0b10000001;
	symbols[1][7] = 0b11111111;

	symbols[2][0] = 0b11011011;
	symbols[2][1] = 0b00000000;
	symbols[2][2] = 0b01100110;
	symbols[2][3] = 0b00000000;
	symbols[2][4] = 0b00011000;
	symbols[2][5] = 0b00011000;
	symbols[2][6] = 0b10000001;
	symbols[2][7] = 0b11111111;

	symbols[3][0] = 0b11111111;
	symbols[3][1] = 0b11111111;
	symbols[3][2] = 0b11111111;
	symbols[3][3] = 0b11111111;
	symbols[3][4] = 0b11111111;
	symbols[3][5] = 0b11111111;
	symbols[3][6] = 0b11111111;
	symbols[3][7] = 0b11111111;

	symbols[4][0] = 0b00000000;
	symbols[4][1] = 0b00000000;
	symbols[4][2] = 0b00000000;
	symbols[4][3] = 0b00000000;
	symbols[4][4] = 0b00000000;
	symbols[4][5] = 0b00000000;
	symbols[4][6] = 0b00000000;
	symbols[4][7] = 0b00000000;

	symbols[5][0] = 0b00000000;
	symbols[5][1] = 0b00000000;
	symbols[5][2] = 0b00000000;
	symbols[5][3] = 0b00000000;
	symbols[5][4] = 0b00000000;
	symbols[5][5] = 0b00000110;
	symbols[5][6] = 0b00000010;
	symbols[5][7] = 0b00000100;

	symbols[6][0] = 0b00000000;
	symbols[6][1] = 0b00011000;
	symbols[6][2] = 0b00011000;
	symbols[6][3] = 0b00011000;
	symbols[6][4] = 0b00011000;
	symbols[6][5] = 0b00000000;
	symbols[6][6] = 0b00011000;
	symbols[6][7] = 0b00000000;

	alphafonts[0][0] = 0b00000000;
	alphafonts[0][1] = 0b00010000;
	alphafonts[0][2] = 0b00101000;
	alphafonts[0][3] = 0b01000100;
	alphafonts[0][4] = 0b01111100;
	alphafonts[0][5] = 0b01000100;
	alphafonts[0][6] = 0b01000100;
	alphafonts[0][7] = 0b00000000;

	alphafonts[1][0] = 0b00000000;
	alphafonts[1][1] = 0b01111000;
	alphafonts[1][2] = 0b01000100;
	alphafonts[1][3] = 0b01111000;
	alphafonts[1][4] = 0b01000100;
	alphafonts[1][5] = 0b01000100;
	alphafonts[1][6] = 0b01111000;
	alphafonts[1][7] = 0b00000000;

	alphafonts[2][0] = 0b00000000;
	alphafonts[2][1] = 0b00111000;
	alphafonts[2][2] = 0b01000100;
	alphafonts[2][3] = 0b01000000;
	alphafonts[2][4] = 0b01000000;
	alphafonts[2][5] = 0b01000100;
	alphafonts[2][6] = 0b00111000;
	alphafonts[2][7] = 0b00000000;

	alphafonts[3][0] = 0b00000000;
	alphafonts[3][1] = 0b01111000;
	alphafonts[3][2] = 0b01000100;
	alphafonts[3][3] = 0b01000010;
	alphafonts[3][4] = 0b01000010;
	alphafonts[3][5] = 0b01000100;
	alphafonts[3][6] = 0b01111000;
	alphafonts[3][7] = 0b00000000;

	alphafonts[4][0] = 0b00000000;
	alphafonts[4][1] = 0b01111100;
	alphafonts[4][2] = 0b01000000;
	alphafonts[4][3] = 0b01000000;
	alphafonts[4][4] = 0b01111100;
	alphafonts[4][5] = 0b01000000;
	alphafonts[4][6] = 0b01111100;
	alphafonts[4][7] = 0b00000000;

	alphafonts[5][0] = 0b00000000;
	alphafonts[5][1] = 0b01111100;
	alphafonts[5][2] = 0b01000000;
	alphafonts[5][3] = 0b01000000;
	alphafonts[5][4] = 0b01111100;
	alphafonts[5][5] = 0b01000000;
	alphafonts[5][6] = 0b01000000;
	alphafonts[5][7] = 0b00000000;

	alphafonts[6][0] = 0b00000000;
	alphafonts[6][1] = 0b00111000;
	alphafonts[6][2] = 0b01000100;
	alphafonts[6][3] = 0b01000000;
	alphafonts[6][4] = 0b01001100;
	alphafonts[6][5] = 0b01000100;
	alphafonts[6][6] = 0b00111000;
	alphafonts[6][7] = 0b00000000;

	alphafonts[7][0] = 0b00000000;
	alphafonts[7][1] = 0b01000100;
	alphafonts[7][2] = 0b01000100;
	alphafonts[7][3] = 0b01000100;
	alphafonts[7][4] = 0b01111100;
	alphafonts[7][5] = 0b01000100;
	alphafonts[7][6] = 0b01000100;
	alphafonts[7][7] = 0b00000000;

	alphafonts[8][0] = 0b00000000;
	alphafonts[8][1] = 0b00111000;
	alphafonts[8][2] = 0b00010000;
	alphafonts[8][3] = 0b00010000;
	alphafonts[8][4] = 0b00010000;
	alphafonts[8][5] = 0b00010000;
	alphafonts[8][6] = 0b00111000;
	alphafonts[8][7] = 0b00000000;

	alphafonts[9][0] = 0b00000000;
	alphafonts[9][1] = 0b00000100;
	alphafonts[9][2] = 0b00000100;
	alphafonts[9][3] = 0b00000100;
	alphafonts[9][4] = 0b00000100;
	alphafonts[9][5] = 0b01000100;
	alphafonts[9][6] = 0b00111000;
	alphafonts[9][7] = 0b00000000;

	alphafonts[10][0] = 0b00000000;
	alphafonts[10][1] = 0b01000100;
	alphafonts[10][2] = 0b01000100;
	alphafonts[10][3] = 0b01001000;
	alphafonts[10][4] = 0b01110000;
	alphafonts[10][5] = 0b01001000;
	alphafonts[10][6] = 0b01000100;
	alphafonts[10][7] = 0b00000000;

	alphafonts[11][0] = 0b00000000;
	alphafonts[11][1] = 0b01000000;
	alphafonts[11][2] = 0b01000000;
	alphafonts[11][3] = 0b01000000;
	alphafonts[11][4] = 0b01000000;
	alphafonts[11][5] = 0b01000000;
	alphafonts[11][6] = 0b00111100;
	alphafonts[11][7] = 0b00000000;


	alphafonts[12][0] = 0b00000000;
	alphafonts[12][1] = 0b10000010;
	alphafonts[12][2] = 0b11000110;
	alphafonts[12][3] = 0b10101010;
	alphafonts[12][4] = 0b10010010;
	alphafonts[12][5] = 0b10000010;
	alphafonts[12][6] = 0b10000010;
	alphafonts[12][7] = 0b00000000;

	alphafonts[13][0] = 0b00000000;
	alphafonts[13][1] = 0b10000100;
	alphafonts[13][2] = 0b11000100;
	alphafonts[13][3] = 0b10100100;
	alphafonts[13][4] = 0b10010100;
	alphafonts[13][5] = 0b10001100;
	alphafonts[13][6] = 0b10000100;
	alphafonts[13][7] = 0b00000000;

	alphafonts[14][0] = 0b00000000;
	alphafonts[14][1] = 0b00111000;
	alphafonts[14][2] = 0b01000100;
	alphafonts[14][3] = 0b01000100;
	alphafonts[14][4] = 0b01000100;
	alphafonts[14][5] = 0b01000100;
	alphafonts[14][6] = 0b00111000;
	alphafonts[14][7] = 0b00000000;

	alphafonts[15][0] = 0b00000000;
	alphafonts[15][1] = 0b00111000;
	alphafonts[15][2] = 0b01000100;
	alphafonts[15][3] = 0b01000100;
	alphafonts[15][4] = 0b01111000;
	alphafonts[15][5] = 0b01000000;
	alphafonts[15][6] = 0b01000000;
	alphafonts[15][7] = 0b00000000;

	alphafonts[16][0] = 0b00000000;
	alphafonts[16][1] = 0b01111000;
	alphafonts[16][2] = 0b10000100;
	alphafonts[16][3] = 0b10000100;
	alphafonts[16][4] = 0b10010100;
	alphafonts[16][5] = 0b10001000;
	alphafonts[16][6] = 0b01110100;
	alphafonts[16][7] = 0b00000010;

	alphafonts[17][0] = 0b00000000;
	alphafonts[17][1] = 0b00111000;
	alphafonts[17][2] = 0b01000100;
	alphafonts[17][3] = 0b01000100;
	alphafonts[17][4] = 0b01111000;
	alphafonts[17][5] = 0b01001000;
	alphafonts[17][6] = 0b01000100;
	alphafonts[17][7] = 0b00000000;

	alphafonts[18][0] = 0b00000000;
	alphafonts[18][1] = 0b00111000;
	alphafonts[18][2] = 0b01000100;
	alphafonts[18][3] = 0b00110000;
	alphafonts[18][4] = 0b00011000;
	alphafonts[18][5] = 0b01000100;
	alphafonts[18][6] = 0b00111000;
	alphafonts[18][7] = 0b00000000;

	alphafonts[19][0] = 0b00000000;
	alphafonts[19][1] = 0b01111100;
	alphafonts[19][2] = 0b00010000;
	alphafonts[19][3] = 0b00010000;
	alphafonts[19][4] = 0b00010000;
	alphafonts[19][5] = 0b00010000;
	alphafonts[19][6] = 0b00010000;
	alphafonts[19][7] = 0b00000000;

	alphafonts[20][0] = 0b00000000;
	alphafonts[20][1] = 0b01000100;
	alphafonts[20][2] = 0b01000100;
	alphafonts[20][3] = 0b01000100;
	alphafonts[20][4] = 0b01000100;
	alphafonts[20][5] = 0b01000100;
	alphafonts[20][6] = 0b00111000;
	alphafonts[20][7] = 0b00000000;

	alphafonts[21][0] = 0b00000000;
	alphafonts[21][1] = 0b01000100;
	alphafonts[21][2] = 0b01000100;
	alphafonts[21][3] = 0b01000100;
	alphafonts[21][4] = 0b01000100;
	alphafonts[21][5] = 0b00101000;
	alphafonts[21][6] = 0b00010000;
	alphafonts[21][7] = 0b00000000;

	alphafonts[22][0] = 0b00000000;
	alphafonts[22][1] = 0b01000010;
	alphafonts[22][2] = 0b01000010;
	alphafonts[22][3] = 0b01000010;
	alphafonts[22][4] = 0b01011010;
	alphafonts[22][5] = 0b01100110;
	alphafonts[22][6] = 0b01000010;
	alphafonts[22][7] = 0b00000000;

	alphafonts[23][0] = 0b01000100;
	alphafonts[23][1] = 0b01000100;
	alphafonts[23][2] = 0b01000100;
	alphafonts[23][3] = 0b00101000;
	alphafonts[23][4] = 0b00010000;
	alphafonts[23][5] = 0b00101000;
	alphafonts[23][6] = 0b01000100;
	alphafonts[23][7] = 0b01000100;

	alphafonts[24][0] = 0b00000000;
	alphafonts[24][1] = 0b01000100;
	alphafonts[24][2] = 0b01000100;
	alphafonts[24][3] = 0b01111100;
	alphafonts[24][4] = 0b00010000;
	alphafonts[24][5] = 0b00010000;
	alphafonts[24][6] = 0b00010000;
	alphafonts[24][7] = 0b00000000;

	alphafonts[25][0] = 0b00000000;
	alphafonts[25][1] = 0b01111100;
	alphafonts[25][2] = 0b00000100;
	alphafonts[25][3] = 0b00001000;
	alphafonts[25][4] = 0b00010000;
	alphafonts[25][5] = 0b00100000;
	alphafonts[25][6] = 0b01111100;
	alphafonts[25][7] = 0b00000000;
	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawPixelsOnLine = 0;
	uint32_t updateScreenMemory = 0;
	uint8_t yCounter = 0;
	uint8_t letterToShow = 0;
	uint8_t TextLine = 0;
	uint8_t xPos = 0;
	uint8_t yPos = 0;

	// looking at the data sheet not sure if you have to do this in 3 instructions or not
	CLKPR = 0b10000000; // set the CLKPCE bit to enable the clock prescaler to be changed
	CLKPR = 0b10000000; // zero the CLKPS3 CLKPS2 CLKPS1 CLKPS0 bits to disable and clock division
	CLKPR = 0b00000000; // clear CLKPCE bit

	clearScreen();

	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10); // switch off clock prescaller
	OCR1A = 1025; 	   //timer interrupt cycles which gives rise to 64usec line sync time
	TCNT1 = 0;
	while(1)
	{
		// wait for hsync timer interrupt to trigger
		while((TIFR1 & (1<<OCF1A)) == 0)
		{
			// wait till the timer overflow flag is SET
		}


		// immediately reset the interrupt timer, previous version had this at the end
		// which made the whole thing be delayed when we're trying to maintain the 64us PAL line timing
		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag

		if (vSync > 0) // invert the line sync pulses when in vsync part of screen
		{
			DDRB = 0;
			for (i = 0; i < HSYNC_BACKPORCH+10; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB = 0;
			DDRB = 1;
		}
		else // hsync
		{
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB = 0;
			PORTB = 1;
			for (i = 0; i < 1; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < HSYNC_BACKPORCH; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1;
			for (i = 0; i < HSYNC_BACKPORCH; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}
		}

		// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec =
		// a delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle

		#define  LUM_ON DDRB = PORTB = 0b11000;
		// cycles i.e. LUM_ON and LUM_OFF must take exactly same time
		//#define  LUM_OFF DDRB = PORTB = 0;
		#define  LUM_OFF DDRB = 0;

		if (drawPixelsOnLine)
		{
			for (i = 0; i < 30; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			register uint8_t * loopPtrMax = &screenMemory[yCounter][XSIZE-1];
			register uint8_t * OneLine = &screenMemory[yCounter][0];
			register uint8_t theBits = *OneLine;

			do {
				if (theBits & 0b10000000) LUM_ON else LUM_OFF
	 			if (theBits & 0b01000000) LUM_ON else LUM_OFF
				if (theBits & 0b00100000) LUM_ON else LUM_OFF
				if (theBits & 0b00010000) LUM_ON else LUM_OFF
				if (theBits & 0b00001000) LUM_ON else LUM_OFF
				if (theBits & 0b00000100) LUM_ON else LUM_OFF
				if (theBits & 0b00000010) LUM_ON else LUM_OFF
				if (theBits & 0b00000001) LUM_ON else LUM_OFF
				OneLine++;
				theBits = *OneLine;
			}while (OneLine < loopPtrMax);
			LUM_OFF

		}



		lineCounter++;
		if (drawPixelsOnLine)
		{
			yCounter++;

			if (printScreen == 0) // draw my name
			{
				putCharXY(0,0,convertToMyCharSet('A'));
				putCharXY(1,7,convertToMyCharSet('D'));
				putCharXY(2,15,convertToMyCharSet('R'));
				putCharXY(3,23,convertToMyCharSet('I'));
				putCharXY(4,31,convertToMyCharSet('A'));
				putCharXY(5,39,convertToMyCharSet('N'));
				putCharXY(0,39+7,convertToMyCharSet('I'));
				putCharXY(1,39+15,convertToMyCharSet('S'));
				putCharXY(0,54+7,convertToMyCharSet('G'));
				putCharXY(1,54+15,convertToMyCharSet('R'));
				putCharXY(2,54+23,convertToMyCharSet('E'));
				putCharXY(3,54+31,convertToMyCharSet('A'));
				putCharXY(4,54+39,convertToMyCharSet('T'));

				updateScreenMemory = 100000;
				printScreen = 128; // only do once
			}
			if (printScreen == 1) // print hello, world
			{
				putCharXY(0,0,convertToMyCharSet('H'));
				putCharXY(0,7,convertToMyCharSet('E'));
				putCharXY(0,15,convertToMyCharSet('L'));
				putCharXY(0,23,convertToMyCharSet('L'));
				putCharXY(0,31,convertToMyCharSet('O'));
				putSymXY(0,38,COMMA);
				putSymXY(0,45,SPACE);
				putCharXY(0,45+0,convertToMyCharSet('W'));
				putCharXY(0,45+7,convertToMyCharSet('O'));
				putCharXY(0,45+15,convertToMyCharSet('R'));
				putCharXY(0,45+23,convertToMyCharSet('L'));
				putCharXY(0,45+31,convertToMyCharSet('D'));
				putSymXY(0,45+45,EXCLAMATION);
				updateScreenMemory = 100000;
				printScreen = 128; // only do once
			}
		}

		switch (lineCounter)
		{
			case 1:
				vSync = 0;
				break;
			case FIRST_LINE_DRAWN+1: drawPixelsOnLine = 1; break;
			//case (MAX_LINE_BEFORE_BLANK - 80) :
			//	drawPixelsOnLine = 1; break;
			//case (MAX_LINE_BEFORE_BLANK - 72) :
			//	drawPixelsOnLine = 0; break;
			case LAST_LINE_DRAWN:
				drawPixelsOnLine = 0; yCounter = 0; LUM_OFF; 	break;
			case (MAX_LINE_BEFORE_BLANK-40):
				drawPixelsOnLine = 0; break;
			case (MAX_LINE_BEFORE_BLANK-6):
				vSync = 1; drawPixelsOnLine = 0; break;
			case MAX_LINE_BEFORE_BLANK:
				lineCounter = 0; vSync = 0;	break;
		}

		//if (scrollDown == MAX_LINE_BEFORE_BLANK - 80)
		//{

		//}
	}
}


#if 0
else
{
	DDRB |= (1 << LUMINANCE_PIN); // pixel on
	PORTB |= (1 << LUMINANCE_PIN); // pixel on
	for (i = 0; i < 205; i++)
	{
		__asm__ __volatile__ ("nop");
	}

	DDRB &= ~(1 << LUMINANCE_PIN); // pixel off
	PORTB &= ~(1 << LUMINANCE_PIN); // pixel off
}

#endif
