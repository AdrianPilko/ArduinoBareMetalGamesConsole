// MIT licence https://opensource.org/license/mit/
// Copyright 2023 Adrian Pilkington

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the Software), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
// IN THE SOFTWARE.




// This is meant to be run on an Arduino UNO board
// pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

// connect pin2 = left
// connect pin3 = right
// conect pin 4 = fire button

/// The code is written deliberately to ensure correct timing, so instead of for or while, do while loops
/// in several places, especially for delays, just directly unrolled loops of asm nop are used (nop = no operation)
/// On arduino a NOP is 1 clock cycles, so gives a timing of
/// 1 / 16MHz = 1 / 16000000 = 0.0000000625seconds = 0.0625microseconds
/// each line of the screen must take 64microseconds
// also switch statements are used to speed up the logic where possible, and more memory is used for
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
#define BASE_ALIEN_OFFSET FIRST_LINE_DRAWN + 4
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
#define YSIZE (HALF_SCREEN * 2)

#define LINE_INC 8
#define LINE_INC_ALIENS 12

int main() {
    int alienDirection = 1;
    int vSync = 0;
    int lineCounter = 0;
    int drawDebug = 0;
    uint8_t alienLineCount = 0;

    typedef struct alienStatusStruct {
        uint8_t alien_row1;
        uint8_t alien_row2;
        uint8_t alien_row3;
        uint8_t alien_row4;
        uint8_t alien_row5;
    }
    alienBitPack_t;

    alienBitPack_t aliensBitPackStatus;

    int alienToggleTrigger = 0;

    int alienToggle = 0;
    int firePressed = 0;
    int fireYPos = 0;
    int fireXPos = 0;

    int alienYBasePos = 0;
    int gameRunning = 0;
    uint8_t kill = 0;
    int outputToneThisLoop = 0;

    typedef enum {
        drawNothing = 0,
            drawAlien_row1,
            drawAlien_row2,
            drawAlien_row3,
            drawAlien_row4,
            drawAlien_row5,
            gameWon,
            gameLost
    }
    drawType_t;
    drawType_t drawType = drawNothing;

    int playerXPos = MIN_X_PLAYER + 68; // has to be non zero and less that 30
    uint8_t alienXStartPos[5] = {
        MIN_X_ALIEN + 2,
        MIN_X_ALIEN + 12,
        MIN_X_ALIEN + 30,
        MIN_X_ALIEN + 45,
        MIN_X_ALIEN + 60
    };
    int alienMoveThisTime = 0;

    uint8_t alienMoveRate = 2;

    int fireRate = 0;

    aliensBitPackStatus.alien_row1 = 0b00011111;
    aliensBitPackStatus.alien_row2 = 0b00011111;
    aliensBitPackStatus.alien_row3 = 0b00011111;
    aliensBitPackStatus.alien_row4 = 0b00011111;
    aliensBitPackStatus.alien_row5 = 0b00011111;

    clock_prescale_set(clock_div_1);

    DDRB |= 1 << COMPOSITE_PIN;
    DDRB |= 1 << LUMINANCE_PIN;

    DDRD &= ~(1 << PD2); //Pin 2 input (move left)
    PORTD |= (1 << PD2); //Pin 2 input

    DDRD &= ~(1 << PD3); //Pin 3 input (move right)
    PORTD |= (1 << PD3); //Pin 3 input

    DDRD &= ~(1 << PD4); //Pin 4 input (fire button)
    PORTD |= (1 << PD4); //Pin 4 input

    DDRB |= (1 << PB1); //Pin 9 output for speaker amp
    PORTB |= (1 << PB1); //Pin 9

    TCCR1B = (1 << CS10); // switch off clock prescaller
    OCR1A = 1024; //timer interrupt cycles which gives rise to 64usec line sync time
    TCNT1 = 0;
    while (1) {
        // wait for hsync timer interrupt to trigger

        while ((TIFR1 & (1 << OCF1A)) == 0) {
            // wait till the timer overflow flag is SET
        }
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
        } else // hsync
        {
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
        switch (drawType) {
        case gameLost: {
            delayLoop(10);
            uint8_t x = 60;
            do {
                PIXEL_ON();
            }
            while (x--> 0);
            PIXEL_OFF();
        }
        break;
        case gameWon: {
            delayLoop(10);
            uint8_t x = 60;
            do {
                PIXEL_ON();
                PIXEL_OFF();
            }
            while (x--> 0);
        }
        break;
        case drawAlien_row1:
            delayLoop(alienXStartPos[0]);
            if (alienToggle == 0) {
                if (aliensBitPackStatus.alien_row1 & 0b00010000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00001000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000100) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000010) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000001) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            } else {
                if (aliensBitPackStatus.alien_row1 & 0b00010000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00001000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000100) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000010) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row1 & 0b00000001) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            }
            break;
        case drawAlien_row2:
            delayLoop(alienXStartPos[0]);
            if (alienToggle == 0) {
                if (aliensBitPackStatus.alien_row2 & 0b00010000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00001000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000100) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000010) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000001) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            } else {
                if (aliensBitPackStatus.alien_row2 & 0b00010000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00001000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000100) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000010) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row2 & 0b00000001) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            }
            break;
        case drawAlien_row3:
            delayLoop(alienXStartPos[0]);
            if (alienToggle == 0) {
                if (aliensBitPackStatus.alien_row3 & 0b00010000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00001000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000100) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000010) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000001) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            } else {
                if (aliensBitPackStatus.alien_row3 & 0b00010000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00001000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000100) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000010) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row3 & 0b00000001) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            }
            break;
        case drawAlien_row4:
            delayLoop(alienXStartPos[0]);
            if (alienToggle == 0) {
                if (aliensBitPackStatus.alien_row4 & 0b00010000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00001000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000100) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000010) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000001) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            } else {
                if (aliensBitPackStatus.alien_row4 & 0b00010000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00001000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000100) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000010) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row4 & 0b00000001) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            }
            break;
        case drawAlien_row5:
            delayLoop(alienXStartPos[0]);
            if (alienToggle == 0) {
                if (aliensBitPackStatus.alien_row5 & 0b00010000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00001000) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000100) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000010) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000001) {
                    alienDraw_1(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            } else {
                if (aliensBitPackStatus.alien_row5 & 0b00010000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00001000) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000100) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000010) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
                if (aliensBitPackStatus.alien_row5 & 0b00000001) {
                    alienDraw_2(alienLineCount);
                } else alienDraw_blank(alienLineCount);
            }
            break;
        }
        if (drawType != drawNothing) {
            if (alienLineCount++ > 7) {
                alienLineCount = 0;
                drawType = drawNothing;
            }
        }

        if (lineCounter > FIRST_LINE_DRAWN + 5) {
            if ((lineCounter == fireYPos) && (firePressed)) {
                delayLoop(fireXPos + 10);
                NOP_FOR_TIMING
                NOP_FOR_TIMING
                NOP_FOR_TIMING
                NOP_FOR_TIMING
                FIVE_NOP_FOR_TIMING
                PIXEL_ON();
                NOP_FOR_TIMING
                PIXEL_OFF();
            }
            if (fireYPos <= FIRST_LINE_DRAWN + 5) {
                firePressed = 0;
            }

            if (outputToneThisLoop > 0) outputToneThisLoop--;

            if (outputToneThisLoop > 0) {
                PORTB |= (1 << PB1); // speaker output
                _delay_us(15);
                PORTB &= ~(1 << PB1);
            }
        }

        if (lineCounter == BASE_ALIEN_Y_1 + alienYBasePos) drawType = drawAlien_row1;
        if (lineCounter == BASE_ALIEN_Y_2 + alienYBasePos) drawType = drawAlien_row2;
        if (lineCounter == BASE_ALIEN_Y_3 + alienYBasePos) drawType = drawAlien_row3;
        if (lineCounter == BASE_ALIEN_Y_4 + alienYBasePos) drawType = drawAlien_row4;
        if (lineCounter == BASE_ALIEN_Y_5 + alienYBasePos) drawType = drawAlien_row5;

        lineCounter++;

        switch (lineCounter) {
        case 1:
            vSync = 0;
            break;
        case FIRST_LINE_DRAWN:

        //PIXEL_ON()
        //delayLoop(160);
        //PIXEL_OFF()
        // debug alien target line up
        {

            TEN_NOP_FOR_TIMING
            PIXEL_ON();
            if (kill >= 1) delayLoop(kill * 5);
            PIXEL_OFF();
        }
        break;
        case FIRST_LINE_DRAWN + 1:
            #if 0
            if (firePressed == 1) {
                if (drawDebug > 0) {
                    delayLoop((5 * drawDebug));
                    NOP_FOR_TIMING
                    NOP_FOR_TIMING
                    PIXEL_ON();
                    delayLoop(10);
                    PIXEL_OFF();
                }
            } else {
                drawDebug = 0;
            }
            #endif

            if ((alienYBasePos > FIRST_LINE_DRAWN + 90) && (drawType != gameWon)) {
                drawType = gameLost;
                outputToneThisLoop = 30000;
            } else if (drawType == gameWon) {
                outputToneThisLoop = 10000;
            } else {
                if (aliensBitPackStatus.alien_row1 == 0 &&
                    aliensBitPackStatus.alien_row2 == 0 &&
                    aliensBitPackStatus.alien_row3 == 0 &&
                    aliensBitPackStatus.alien_row4 == 0 &&
                    aliensBitPackStatus.alien_row5 == 0) {
                    drawType = gameWon;
                }

                // Check if input control pins
                if (PIND & (1 << PD2)) {
                    playerXPos = playerXPos + 1;
                }
                if (PIND & (1 << PD3)) {
                    playerXPos = playerXPos - 1;
                }
                if (PIND & (1 << PD4)) {

                } else {
                    if (firePressed == 0) {
                        firePressed = 1;
                        fireYPos = MAX_LINE_BEFORE_BLANK - 66;
                        fireXPos = playerXPos;
                        gameRunning = 1;
                        //outputToneThisLoop = 10000;
                    }
                }

                if (playerXPos > MAX_X_PLAYER) {
                    playerXPos = MAX_X_PLAYER - 1;
                }
                if (playerXPos < MIN_X_PLAYER) {
                    playerXPos = MIN_X_PLAYER + 1;
                }

                if (fireXPos == alienXStartPos[0]) {
                    drawDebug = 1;
                }
                if (fireXPos == alienXStartPos[1]) {
                    drawDebug = 2;
                }
                if (fireXPos == alienXStartPos[2]) {
                    drawDebug = 3;
                }
                if (fireXPos == alienXStartPos[3]) {
                    drawDebug = 4;
                }
                if (fireXPos == alienXStartPos[4]) {
                    drawDebug = 5;
                }
                /// all the alien init gumbins!

                alienMoveThisTime = alienMoveThisTime + 1;

                if (gameRunning == 1) {
                    if (alienMoveThisTime >= alienMoveRate) {

                        if (alienDirection == 1) {
                            alienXStartPos[0] += alienMoveRate;
                            alienXStartPos[1] = alienXStartPos[0] + 12;
                            alienXStartPos[2] = alienXStartPos[0] + 30;
                            alienXStartPos[3] = alienXStartPos[0] + 45;
                            alienXStartPos[4] = alienXStartPos[0] + 60;
                        } else {
                            alienXStartPos[0] -= alienMoveRate;
                            alienXStartPos[1] = alienXStartPos[0] + 12;
                            alienXStartPos[2] = alienXStartPos[0] + 30;
                            alienXStartPos[3] = alienXStartPos[0] + 45;
                            alienXStartPos[4] = alienXStartPos[0] + 60;
                        }
                        alienMoveThisTime = 0;
                    }
                }
                //alienXStartPos[0] = playerXPos;

                if (alienXStartPos[0] > MAX_X_ALIEN - alienMoveRate) // remember that this is only the X position of left most alien
                {
                    alienDirection = 0;
                    // move aliens down by one line (getting closer to you!)
                    alienYBasePos += 1;
                }
                if (alienXStartPos[0] < MIN_X_ALIEN + alienMoveRate) {
                    alienDirection = 1;
                    alienXStartPos[0] = MIN_X_ALIEN + 2;
                    alienXStartPos[1] = MIN_X_ALIEN + 12;
                    alienXStartPos[2] = MIN_X_ALIEN + 30;
                    alienXStartPos[3] = MIN_X_ALIEN + 45;
                    alienXStartPos[4] = MIN_X_ALIEN + 60;

                    // move aliens down by one line (getting closer to you!)
                    alienYBasePos += 1;
                    outputToneThisLoop = 10000;

                    // speed up the aliens
                    //alienMoveRate = alienMoveRate + 2;
                    //if (alienMoveRate >= 2)
                    //{
                    //	alienMoveRate = 2;
                    //}
                }

                if (alienToggleTrigger++ >= 20 - alienMoveRate) {
                    alienToggleTrigger = 0;
                    alienToggle = 1 - alienToggle;
                }
                if (fireRate-- == 0) {
                    fireYPos -= 8;
                    fireRate = 2;
                }
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
        case (MAX_LINE_BEFORE_BLANK - 64): /// code to draw player
            delayLoop(playerXPos);
            PIXEL_ON();
            delayLoop(PLAYER_WIDTH);
            PIXEL_OFF_NO_NOP();
            break;
        case (MAX_LINE_BEFORE_BLANK - 62): // second line of player
            delayLoop(playerXPos - 5);
            PIXEL_ON();
            delayLoop(PLAYER_WIDTH);
            PIXEL_OFF_NO_NOP();
            break;
        case (MAX_LINE_BEFORE_BLANK - 60): // last line of the player draw
            delayLoop(playerXPos - 5);
            PIXEL_ON();
            delayLoop(PLAYER_WIDTH);
            PIXEL_OFF_NO_NOP();
            break;
        case (MAX_LINE_BEFORE_BLANK - 50):
            break;
        case (MAX_LINE_BEFORE_BLANK - 48):
            TEN_NOP_FOR_TIMING
            PIXEL_ON()
            delayLoop(150);
            PIXEL_OFF()
            if (firePressed) {
                if ((fireXPos >= alienXStartPos[0]) && (fireXPos - 4 <= alienXStartPos[0]) && (fireYPos >= BASE_ALIEN_Y_1 + alienYBasePos) && (fireYPos - 20 < BASE_ALIEN_Y_1 + alienYBasePos)) {
                    if (aliensBitPackStatus.alien_row5 & 0b00010000) {
                        aliensBitPackStatus.alien_row5 &= ~(0b00010000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row4 & 0b00010000) {
                        aliensBitPackStatus.alien_row4 &= ~(0b00010000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row3 & 0b00010000) {
                        aliensBitPackStatus.alien_row3 &= ~(0b00010000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row2 & 0b00010000) {
                        aliensBitPackStatus.alien_row2 &= ~(0b00010000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row1 & 0b00010000) {
                        aliensBitPackStatus.alien_row1 &= ~(0b00010000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    }
                } else if ((fireXPos >= alienXStartPos[1]) && (fireXPos - 4 < alienXStartPos[1]) &&
                    (fireYPos >= BASE_ALIEN_Y_2 + alienYBasePos) && (fireYPos - 20 < BASE_ALIEN_Y_2 + alienYBasePos)) {
                    if (aliensBitPackStatus.alien_row5 & 0b00001000) {
                        aliensBitPackStatus.alien_row5 &= ~(0b00001000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row4 & 0b00001000) {
                        aliensBitPackStatus.alien_row4 &= ~(0b00001000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row3 & 0b00001000) {
                        aliensBitPackStatus.alien_row3 &= ~(0b00001000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row2 & 0b00001000) {
                        aliensBitPackStatus.alien_row2 &= ~(0b00001000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row1 & 0b00001000) {
                        aliensBitPackStatus.alien_row1 &= ~(0b00001000);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    }

                } else if ((fireXPos >= alienXStartPos[2]) && (fireXPos - 4 < alienXStartPos[2]) &&
                    (fireYPos >= BASE_ALIEN_Y_3 + alienYBasePos) && (fireYPos - 20 < BASE_ALIEN_Y_3 + alienYBasePos)) {
                    if (aliensBitPackStatus.alien_row5 & 0b00000100) {
                        aliensBitPackStatus.alien_row5 &= ~(0b00000100);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row4 & 0b00000100) {
                        aliensBitPackStatus.alien_row4 &= ~(0b00000100);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row3 & 0b00000100) {
                        aliensBitPackStatus.alien_row3 &= ~(0b00000100);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row2 & 0b00000100) {
                        aliensBitPackStatus.alien_row2 &= ~(0b00000100);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row1 & 0b00000100) {
                        aliensBitPackStatus.alien_row1 &= ~(0b00000100);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    }
                } else if ((fireXPos >= alienXStartPos[3]) && (fireXPos - 4 < alienXStartPos[3]) &&
                    (fireYPos >= BASE_ALIEN_Y_4 + alienYBasePos) && (fireYPos - 20 < BASE_ALIEN_Y_4 + alienYBasePos)) {
                    if (aliensBitPackStatus.alien_row5 & 0b00000010) {
                        aliensBitPackStatus.alien_row5 &= ~(0b00000010);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row4 & 0b00000010) {
                        aliensBitPackStatus.alien_row4 &= ~(0b00000010);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row3 & 0b00000010) {
                        aliensBitPackStatus.alien_row3 &= ~(0b00000010);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row2 & 0b00000010) {
                        aliensBitPackStatus.alien_row2 &= ~(0b00000010);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row1 & 0b00000010) {
                        aliensBitPackStatus.alien_row1 &= ~(0b00000010);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    }
                } else if ((fireXPos >= alienXStartPos[4]) && (fireXPos - 4 < alienXStartPos[4]) &&
                    (fireYPos >= BASE_ALIEN_Y_5 + alienYBasePos) && (fireYPos - 20 < BASE_ALIEN_Y_5 + alienYBasePos)) {
                    if (aliensBitPackStatus.alien_row5 & 0b00000001) {
                        aliensBitPackStatus.alien_row5 &= ~(0b00000001);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row4 & 0b00000001) {
                        aliensBitPackStatus.alien_row4 &= ~(0b00000001);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row3 & 0b00000001) {
                        aliensBitPackStatus.alien_row3 &= ~(0b00000001);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row2 & 0b00000001) {
                        aliensBitPackStatus.alien_row2 &= ~(0b00000001);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    } else if (aliensBitPackStatus.alien_row1 & 0b00000001) {
                        aliensBitPackStatus.alien_row1 &= ~(0b00000001);
                        firePressed = 0;
                        kill++;
                        outputToneThisLoop = 30000;
                    }
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
