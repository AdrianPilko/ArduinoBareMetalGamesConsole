//#define DEBUG_PIN5_MEASURE_ON_INT_LOOP
//#define DEBUG_PIN5_MEASURE_ON_ALIEN_DELAY
#define DEBUG_PIN5_MEASURE_HSYNC

// This is meant to be run on an Arduino UNO board
// pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

// connect pin2 3 and 4 through ground for right, left and fire button

/// The code is written deliberately to ensure correct timing, so instead of for or while, do while loops
/// in several places, especially for delays, just directly unrolled loops of asm nop are used (nop = no operation)
/// On arduino a NOP is 1 clock cycles, so gives a timing of
/// 1 / 16MHz = 1 / 16000000 = 0.0000000625seconds = 0.0625microseconds
/// each line of the screen must take 64microseconds
// also switch staements are used to speed up the logic where possible, and more memory is used for
// structures to save execution time - all this to say the code may look messy but try doing it other ways
// without using just assembly code (might be the final option!)

#include <avr/power.h>
#include<avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "charset.h"
#include "asmUtil.h"

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 300
#define FIRST_LINE_DRAWN 50
#define LAST_LINE_DRAWN 262
#define HALF_SCREEN 110
#define PLAYER_WIDTH 16
#define BARRIER_WIDTH 20
#define MIN_DELAY 5
#define BASE_ALIEN_OFFSET FIRST_LINE_DRAWN+4
#define BASE_ALIEN_Y_1 (60)
#define BASE_ALIEN_Y_2 (70)
#define BASE_ALIEN_Y_3 (80)
#define BASE_ALIEN_Y_4 (90)
#define BASE_ALIEN_Y_5 (100)
#define BASE_ALIEN_Y_6 (110)

#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15
#define INITIAL_ALIEN_MOVE_RATE 10

#define WIDTH_ALL_ALIENS 30
#define MAX_X_PLAYER 160
#define MIN_X_PLAYER 3
#define MAX_X_ALIEN 90
#define MIN_X_ALIEN 2

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

#define LINE_INC 8
#define LINE_INC_ALIENS 12

int main()
{
	int alienDirection = 1;
	int vSync = 0;
	int lineCounter = 0;
	int lineValidForFire = 0;
	uint8_t alienLineCount = 0;

	typedef struct alienStatusStruct
	{
		uint8_t alien_row1;
		uint8_t alien_row2;
		uint8_t alien_row3;
		uint8_t alien_row4;
		uint8_t alien_row5;
	} alienBitPack_t;


	alienBitPack_t aliensBitPackStatus;

	int alienToggleTrigger = 0;

	int alienToggle = 0;
	int firePressed = 0;
	int alienYBasePos = 0;
	int gameRunning = 0;

	typedef enum {drawNothing=0,
		drawAlien_row1,
		drawAlien_row2,
		drawAlien_row3,
		drawAlien_row4,
		drawAlien_row5,
		gameWon,
		gameLost} drawType_t;
	drawType_t drawType = drawNothing;
	int playerXPos = MIN_X_PLAYER+68;  // has to be non zero and less that 30
	uint8_t  alienXStartPos[5] = {MIN_X_ALIEN+2,
			                 MIN_X_ALIEN+4,
							 MIN_X_ALIEN+6,
							 MIN_X_ALIEN+8,
							 MIN_X_ALIEN+10};
	int alienMoveThisTime = 0;

	uint8_t alienMoveRate = 1;

	uint8_t alienCount = 5 * 5;
	int fireRate = 0;


	aliensBitPackStatus.alien_row1 = 0b00011111;
	aliensBitPackStatus.alien_row2 = 0b00011111;
	aliensBitPackStatus.alien_row3 = 0b00011111;
	aliensBitPackStatus.alien_row4 = 0b00011111;
	aliensBitPackStatus.alien_row5 = 0b00011111;

	clock_prescale_set(clock_div_1);

	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	DDRD &= ~(1 << PD2);	//Pin 2 input (move right)
	PORTD |= (1 << PD2);    //Pin 2 input

	DDRD &= ~(1 << PD3);	//Pin 3 input (move left)
	PORTD |= (1 << PD3);    //Pin 3 input

	DDRD &= ~(1 << PD4);	//Pin 4 input (fire button)
	PORTD |= (1 << PD4);    //Pin 4 input

	DDRD |= (1 << PD5);	//Pin 5 output for debug (measure length of pulse on scope)
	PORTD |= (1 << PD5);    //Pin 5

	TCCR1B = (1 << CS10); // switch off clock prescaller
	OCR1A = 1024; //timer interrupt cycles which gives rise to 64usec line sync time
	TCNT1 = 0;
	while (1) {
		// wait for hsync timer interrupt to trigger

		while ((TIFR1 & (1 << OCF1A)) == 0) {
			// wait till the timer overflow flag is SET
		}

#ifdef DEBUG_PIN5_MEASURE_ON_INT_LOOP

		PORTD |= (1 << PD5); // Set bit 5 of PORTD to 1 (high) this will tell me (via a scope on the pin ) how long we're waiting here
		                   // which means how long we have left for our other code
#endif

#ifdef DEBUG_PIN5_MEASURE_ON_INT_LOOP
		PORTD &= ~(1 << PD5);
#endif

		// immediately reset the interrupt timer, previous version had this at the end
		// which made the whole thing be delayed when we're trying to maintain the 64us PAL line timing
		TCNT1 = 0;
		TIFR1 |= (1 << OCF1A); //clear timer1 overflow flag

		if (vSync > 0) // invert the line sync pulses when in vsync part of screen
				{
			DDRB = 0;
			for (int i = 0; i < HSYNC_BACKPORCH + 10; i++) {
				NOP_FOR_TIMING
			}
			PORTB = 0;
			DDRB = 1;
		}
		else // hsync
		{
#ifdef DEBUG_PIN5_MEASURE_HSYNC

		PORTD |= (1 << PD5); // Set bit 5 of PORTD to 1 (high) this will tell me (via a scope on the pin ) how long we're waiting here
		                   // which means how long we have left for our other code
#endif
#ifdef DEBUG_PIN5_MEASURE_HSYNC
		PORTD &= ~(1 << PD5);
#endif
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB = 0;
			PORTB = 1;
			for (int i = 0; i < 1; i++) {
				NOP_FOR_TIMING
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (int i = 0; i < HSYNC_BACKPORCH; i++) {
				NOP_FOR_TIMING
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1;
			for (int i = 0; i < HSYNC_BACKPORCH; i++) // delay same amount to give proper back porch before drawing any pixels on the line
					{
				NOP_FOR_TIMING
			}
		}


		// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec =
		// a delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle
		//drawType = drawNothing;
		switch (drawType)
		{
			case gameLost: PIXEL_ON();
						   PIXEL_OFF();
						   PIXEL_ON();
						   PIXEL_OFF();
				break;
			case drawAlien_row1:
				delayLoop(alienXStartPos[0]);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row1 & 0b00010000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00001000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000100)
					{
						alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000010)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000001)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row1 & 0b00010000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00001000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000100)
					{
						alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000010)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1 & 0b00000001)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				break;
			case drawAlien_row2:
				delayLoop(alienXStartPos[0]);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row2 & 0b00010000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00001000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000100)
					{
						alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000010)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000001)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row2 & 0b00010000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00001000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000100)
					{
						alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000010)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2 & 0b00000001)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				break;
			case drawAlien_row3:
				delayLoop(alienXStartPos[0]);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row3 & 0b00010000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00001000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000100)
					{
						alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000010)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000001)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row3 & 0b00010000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00001000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000100)
					{
						alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000010)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3 & 0b00000001)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				break;
			case drawAlien_row4:
				delayLoop(alienXStartPos[0]);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row4 & 0b00010000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00001000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000100)
					{
						alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000010)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000001)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row4 & 0b00010000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00001000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000100)
					{
						alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000010)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4 & 0b00000001)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				break;
			case drawAlien_row5:
				delayLoop(alienXStartPos[0]);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row5 & 0b00010000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00001000)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000100)
					{
						alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000010)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000001)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row5 & 0b00010000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00001000)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000100)
					{
						alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000010)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5 & 0b00000001)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				break;
		}
		if (drawType != drawNothing)
		{
			if (alienLineCount++ > 7)
			{
				alienLineCount = 0;
				drawType = drawNothing;
			}
		}
		//if ((BASE_ALIEN_Y_5+alienYBasePos > MAX_LINE_BEFORE_BLANK - 64))
//		{
			//drawType = gameLost;
		//	alienYBasePos = 0;
	//	}
		//else if (drawType == gameWon)
		//if (drawType == gameWon)
		//{
			// do nothing
		//}
		//else
		// row_1 is top most

		//if (drawType != gameLost)
		//{
			if (lineCounter == BASE_ALIEN_Y_1+alienYBasePos) drawType = drawAlien_row1;
			if (lineCounter == BASE_ALIEN_Y_2+alienYBasePos) drawType = drawAlien_row2;
			if (lineCounter == BASE_ALIEN_Y_3+alienYBasePos) drawType = drawAlien_row3;
			if (lineCounter == BASE_ALIEN_Y_4+alienYBasePos) drawType = drawAlien_row4;
			if (lineCounter == BASE_ALIEN_Y_5+alienYBasePos) drawType = drawAlien_row5;
		//}

		lineCounter++;

		switch (lineCounter)
		{
		case 1:
			vSync = 0;
			firePressed = 0;
			break;
		case FIRST_LINE_DRAWN:
			PIXEL_ON()
			delayLoop(160);
			PIXEL_OFF()
			break;
		case FIRST_LINE_DRAWN+1:
			if ((alienYBasePos > FIRST_LINE_DRAWN+60) && (drawType != gameWon))
			{
				drawType = gameLost;
			}
			else if (drawType == gameWon)
			{
				// do nothing
			}
			else
			{
				// Check if input control pins
				if (PIND & (1 << PD2)) {
					playerXPos = playerXPos - 2;
				}
				if (PIND & (1 << PD3)) {
					playerXPos = playerXPos + 2;
				}
				if (PIND & (1 << PD4))
				{

				}
				else
				{
					if (fireRate > 12)
					{
						firePressed = 1;
						fireRate = 0;
					}
					gameRunning = 1;
				}

				if (playerXPos > MAX_X_PLAYER) {
					playerXPos = MAX_X_PLAYER-1;
				}
				if (playerXPos < MIN_X_PLAYER) {
					playerXPos = MIN_X_PLAYER+1;
				}
				/// all the alien init gumbins!

				alienMoveThisTime = alienMoveThisTime + 1;

				if (gameRunning)
				{
					if (alienMoveThisTime >= alienMoveRate)
					{
						if (alienDirection == 1)
						{
							alienXStartPos[0]+=alienMoveRate;
							alienXStartPos[1]=alienXStartPos[0]+2;
							alienXStartPos[2]=alienXStartPos[1]+2;
							alienXStartPos[3]=alienXStartPos[2]+2;
							alienXStartPos[4]=alienXStartPos[3]+2;
						}
						else
						{
							alienXStartPos[0]-=alienMoveRate;
							alienXStartPos[1]=alienXStartPos[0]+2;
							alienXStartPos[2]=alienXStartPos[1]+2;
							alienXStartPos[3]=alienXStartPos[2]+2;
							alienXStartPos[4]=alienXStartPos[3]+2;
						}
						alienMoveThisTime = 0;
					}
				}
				//alienXStartPos[0] = playerXPos;


				if (alienXStartPos[0] > MAX_X_ALIEN-alienMoveRate) // remember that this is only the X position of left most alien
				{
					alienDirection = 0;
					// move aliens down by one line (getting closer to you!)
					alienYBasePos += 1;
				}
				if (alienXStartPos[0] < MIN_X_ALIEN+alienMoveRate)
				{
					alienDirection = 1;
					alienXStartPos[0]=MIN_X_ALIEN+2;
					alienXStartPos[1]=alienXStartPos[0]+4;
					alienXStartPos[2]=alienXStartPos[0]+6;
					alienXStartPos[3]=alienXStartPos[0]+8;
					alienXStartPos[4]=alienXStartPos[0]+10;
					// move aliens down by one line (getting closer to you!)
					alienYBasePos += 1;

					// speed up the aliens
					alienMoveRate = alienMoveRate + 1;
					if (alienMoveRate > 4)
					{
						alienMoveRate = 4;
					}
				}

				if (alienToggleTrigger++ >= 20 - alienMoveRate)
				{
					alienToggleTrigger = 0;
					alienToggle = 1 - alienToggle;
				}
				fireRate++;
			}
			break;

		case (MAX_LINE_BEFORE_BLANK - 120):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 112):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 104):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 96):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;

		case (MAX_LINE_BEFORE_BLANK - 88):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 80):
			//TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			FIVE_NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 77):
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			NOP_FOR_TIMING;
		    NOP_FOR_TIMING;
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 66):    /// code to fire
			if (firePressed == 1)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 65):    /// code to fire
			if (firePressed == 1)
			{
				TEN_NOP_FOR_TIMING;
				FIVE_NOP_FOR_TIMING;
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 64):    /// code to draw player
			delayLoop(playerXPos);
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			PIXEL_OFF_NO_NOP();
			lineValidForFire = 0;
			break;
		case (MAX_LINE_BEFORE_BLANK - 63):
			break;
		case (MAX_LINE_BEFORE_BLANK - 62):
			delayLoop(playerXPos);
			TEN_NOP_FOR_TIMING;
			FIVE_NOP_FOR_TIMING;
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			PIXEL_OFF_NO_NOP();
			lineValidForFire = 0;
			break;
		case (MAX_LINE_BEFORE_BLANK - 61):
			break;
		case (MAX_LINE_BEFORE_BLANK - 60):  // last line of the player draw
			delayLoop(playerXPos-2);
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			TEN_NOP_FOR_TIMING;
			FIVE_NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 50):
			break;
		case (MAX_LINE_BEFORE_BLANK - 48):
			PIXEL_ON()
			delayLoop(160);
			PIXEL_OFF()
			if (firePressed)
			{
				if (playerXPos == alienXStartPos[0])
				{
					if (aliensBitPackStatus.alien_row5 &      0b00010000) { aliensBitPackStatus.alien_row5 &= ~(0b00010000);}
					else if (aliensBitPackStatus.alien_row4 & 0b00010000) { aliensBitPackStatus.alien_row4 &= ~(0b00010000);}
					else if (aliensBitPackStatus.alien_row3 & 0b00010000) { aliensBitPackStatus.alien_row3 &= ~(0b00010000);}
					else if (aliensBitPackStatus.alien_row2 & 0b00010000) { aliensBitPackStatus.alien_row2 &= ~(0b00010000);}
					else if (aliensBitPackStatus.alien_row1 & 0b00010000) { aliensBitPackStatus.alien_row1 &= ~(0b00010000);}
				}
				else if (playerXPos == alienXStartPos[1])
				{
					if (aliensBitPackStatus.alien_row5 &      0b00001000) { aliensBitPackStatus.alien_row5 &= ~(0b00001000);}
					else if (aliensBitPackStatus.alien_row4 & 0b00001000) { aliensBitPackStatus.alien_row4 &= ~(0b00001000);}
					else if (aliensBitPackStatus.alien_row3 & 0b00001000) { aliensBitPackStatus.alien_row3 &= ~(0b00001000);}
					else if (aliensBitPackStatus.alien_row2 & 0b00001000) { aliensBitPackStatus.alien_row2 &= ~(0b00001000);}
					else if (aliensBitPackStatus.alien_row1 & 0b00001000) { aliensBitPackStatus.alien_row1 &= ~(0b00001000);}

				}
				else if (playerXPos == alienXStartPos[2])
				{
					if (aliensBitPackStatus.alien_row5 &      0b00000100) { aliensBitPackStatus.alien_row5 &= ~(0b00000100);}
					else if (aliensBitPackStatus.alien_row4 & 0b00000100) { aliensBitPackStatus.alien_row4 &= ~(0b00000100);}
					else if (aliensBitPackStatus.alien_row3 & 0b00000100) { aliensBitPackStatus.alien_row3 &= ~(0b00000100);}
					else if (aliensBitPackStatus.alien_row2 & 0b00000100) { aliensBitPackStatus.alien_row2 &= ~(0b00000100);}
					else if (aliensBitPackStatus.alien_row1 & 0b00000100) { aliensBitPackStatus.alien_row1 &= ~(0b00000100);}
				}
				else if (playerXPos == alienXStartPos[3])
				{
					if (aliensBitPackStatus.alien_row5 &      0b00000010) { aliensBitPackStatus.alien_row5 &= ~(0b00000010);}
					else if (aliensBitPackStatus.alien_row4 & 0b00000010) { aliensBitPackStatus.alien_row4 &= ~(0b00000010);}
					else if (aliensBitPackStatus.alien_row3 & 0b00000010) { aliensBitPackStatus.alien_row3 &= ~(0b00000010);}
					else if (aliensBitPackStatus.alien_row2 & 0b00000010) { aliensBitPackStatus.alien_row2 &= ~(0b00000010);}
					else if (aliensBitPackStatus.alien_row1 & 0b00000010) { aliensBitPackStatus.alien_row1 &= ~(0b00000010);}
				}
				else if (playerXPos == alienXStartPos[4])
				{
					if (aliensBitPackStatus.alien_row5 &      0b00000001) { aliensBitPackStatus.alien_row5 &= ~(0b00000001);}
					else if (aliensBitPackStatus.alien_row4 & 0b00000001) { aliensBitPackStatus.alien_row4 &= ~(0b00000001);}
					else if (aliensBitPackStatus.alien_row3 & 0b00000001) { aliensBitPackStatus.alien_row3 &= ~(0b00000001);}
					else if (aliensBitPackStatus.alien_row2 & 0b00000001) { aliensBitPackStatus.alien_row2 &= ~(0b00000001);}
					else if (aliensBitPackStatus.alien_row1 & 0b00000001) { aliensBitPackStatus.alien_row1 &= ~(0b00000001);}
				}
			}

			break;
		case (MAX_LINE_BEFORE_BLANK - 6):
			vSync = 1;
			break;
		case MAX_LINE_BEFORE_BLANK:
			lineCounter = 0;
			vSync = 0;
			break;
		}
		//_delay_us(24.0); using a scope and the pin 5 output high low around the interrrupt timer loop 23 or 24usec delay here results in
		// tv hsync issues sync because the scopes showing less than 180nsec left at worst case so the timing can't be maintained
		/// as the timer interrupt has already expired
	}
}
