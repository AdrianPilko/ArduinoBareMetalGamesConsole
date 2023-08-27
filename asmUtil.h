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
