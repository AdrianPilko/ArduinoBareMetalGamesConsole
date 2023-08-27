#define PIXEL_OFF() \
		asm volatile ( \
		 "ldi r16, 0\n" \
		 "out %[port1], r16\n" \
		 "nop\n"    \
		 "nop\n"    \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(DDRB)) \
		 );
#define PIXEL_OFF_NO_NOP() \
		asm volatile ( \
		 "ldi r16, 0\n" \
		 "out %[port1], r16\n" \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(DDRB)) \
		 );


#define PIXEL_ON() \
		asm volatile ( \
		 "ldi r16, 0b11000\n" \
		 "out %[port1], r16\n" \
		 "out %[port2], r16\n"    \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(PORTB)), [port2] "I" (_SFR_IO_ADDR(DDRB)) \
		 );
#define NOP_FOR_TIMING asm volatile ("nop");

void alienDraw_1(const uint8_t alienLineCount)
{
	switch (alienLineCount)
	{
		case 0:
			PIXEL_ON() //0b10000001,
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()break;
		case 1:
			PIXEL_OFF() //0b01111110,
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 2:
			NOP_FOR_TIMING //01011010
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 3:
			NOP_FOR_TIMING//0b01111110,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 4:
			NOP_FOR_TIMING  //0b00111100,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 5: // legs
			PIXEL_OFF() //0b00100100,
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			break;
		case 6:
			PIXEL_OFF() //	0b01000010,
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 7:
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()  //	0b10000001}
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			break;
		default:PIXEL_OFF_NO_NOP() break;
	};
}


void alienDraw_2(const uint8_t alienLineCount)
{
	switch (alienLineCount)
	{
		case 0:   // ears
			PIXEL_OFF_NO_NOP()//0b01000010,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			break;
		case 1:  // top of head
			PIXEL_OFF() //0b01111110,
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING

			break;
		case 2: // eyes
			NOP_FOR_TIMING //01011010
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
//			NOP_FOR_TIMING
			break;
		case 3: // mouth
			NOP_FOR_TIMING//0b01111110,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			break;
		case 4: // bottom of body
			NOP_FOR_TIMING  //0b00111100,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 5:   // the legs
			PIXEL_OFF() //0b00100100,
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 6:
			PIXEL_OFF() //	0b01000010,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 7:
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		default:PIXEL_OFF_NO_NOP() break;
	};
}


#if 0
		        // Check if Pin 2 is high
		        if (PIND & (1 << PD2))
			    {
		        	playerXPos = playerXPos - 1;
		        }
		        // Check if Pin 3 is high
		        if (PIND & (1 << PD3))
		        {
		        	playerXPos = playerXPos + 1;
		        }
#endif
