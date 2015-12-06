#define F_CPU 1000000UL
#define F_OSC 1000000

#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

void blink() {
	// set port B for output
	DDRB = 0xff;

	// Blink everything connected to port B
	while(true) {
		PORTB = 0xff;

		_delay_ms(500);

		PORTB = 0x00;

		_delay_ms(500);
	}
}

inline void srSetup() {
	DDRC |= 0b00000111;
	PORTC = 0b00000000;
}

inline void srClock() {
	PORTC ^= 0b00000001;
	PORTC ^= 0b00000001;
}

inline void srLatch() {
	PORTC ^= 0b00000100;
	PORTC ^= 0b00000100;
}

inline void writeLatch(uint8_t data) {
	for(int i = 0; i < 8; ++i) {
		PORTC &= ~0b00000010;
		if(data & 0b00000001) {
			PORTC |= 0b00000010;
		}
		
		srClock();
		
		data >>= 1;
	}
}

// R 0b11000000
// G 0b00110000
// B 0b00001100
//volatile uint8_t mt[4][8][8];

/**
 * Framebuffer
 * 
 * mt[ROW][COLUMN][COLOR] = INTENSITY (8bit)
 */
uint8_t mt[16][8][3];

const uint8_t MAX_INTENSITY = 7;
const uint8_t MAX_ROW = 7;
const uint8_t MAX_COLLUMN = 7;


/**
 * Timer interrupt
 * 
 * Display output for this period of Timer
 * 
 */
uint8_t row = 0;
uint8_t intensity = 0;
SIGNAL(TIMER1_COMPA_vect) {
	// Write blue byte
/*	uint8_t blue = 0;
	for(int col = 0; col < 8; col++) {
		if(mt[row][col][2] > intensity) {
			blue |= (1 << col);
		}
	}
	for(int col = 0; col < 8; col++) {
		if(mt[row][col][2] > intensity) {
			blue |= (1 << col);
		}
	}
	for(int col = 0; col < 8; col++) {
		if(mt[row][col][2] > intensity) {
			blue |= (1 << col);
		}
	}
	/*
	if(intensity < 32) {
		blue = 0xff; 
	}*/
	
/*	writeLatch(~blue);
	writeLatch(~blue);
	writeLatch(~blue);*/

	writeLatch(mt[intensity][row][0]);
	writeLatch(mt[intensity][row][1]);
	writeLatch(mt[intensity][row][2]);
	
	// Set current row
	PORTD = 0;
	srLatch();
	PORTD = 1 << row;
	
//	writeLatch(0x00);
	
	// Timer strobe
/*	PORTC ^= 0b00010000;
	_delay_ms(5);
	PORTC ^= ~0b00010000;*/
	
	row++;
	if(row > MAX_ROW) {
		intensity++;
		row = 0;
	}
	
	if(intensity > MAX_INTENSITY) {
		intensity = 0;
	}
}

int main (void) {
	DDRC = 0b00010111;
	
	DDRD = 0b11111111;
	PORTD = 0b11111111;
	
	// Boot blink
	PORTC |= 0b00010000;
	_delay_ms(500);
	PORTC &= ~0b00010000;
		
	
	// Clear screen
	srSetup();
	writeLatch(0b11111111);
	writeLatch(0b11111111);
	writeLatch(0b11111111);
	
	
	// Enable TIMER1 interrupt - plane switching
	OCR1A  = 1; // Number to count up to
	// COM1A1 COM1A0 COM1B1 COM1B0 – – WGM11 WGM10
	TCCR1A = 0b0000'0001; // Clear Timer on Compare Match (CTC) mode
	// ICNC1 ICES1 – WGM13 WGM12 CS12 CS11 CS10 
	TCCR1B = 0b0001'0011; // clock source CLK/1024, start timer
	TIFR1 |= 1 << 6; // clear interrupt flag
	TIMSK1 |= 1 << OCIE1A; // TC0 compare match A interrupt enable
	sei();
	
/*	uint8_t data = 0;
	while(true) {
		writeLatch(0b00000000);
		_delay_ms(100);
	}*/

/*	while(true) {
		for(int r = 0; r < 8; ++r) {
			PORTD = 1 << r;
			
			for(int c = 0; c < 255; c++) {
				writeLatch(~c);
				_delay_ms(25);
			}
		}
	}*/
	
/*	while(true) {
		_delay_ms(25);
		
		for(int i = 0; i < 8; ++i) {
			mt[i][i][2]++;
		}
	}*/
	
	for(int i = 0 ; i <= MAX_INTENSITY; i ++) {
		for(int r = 0 ; r <= MAX_ROW; r ++) {
			mt[i][r][0] = 0b11111111;
			mt[i][r][1] = 0b11111111;
			mt[i][r][2] = 0b11111111;
		}
	}
	
	uint8_t ROW2INT[] = {0, 1, 2, 3, 4, 5, 6, 7};
	
	for(int r = 0; r < 8; ++r) {
		const uint8_t intensity = ROW2INT[r];
		for(int i = 0; i < intensity; ++i) {
			mt[i][r][2] = ~(1 << r);
		}
	}

	while(true);
}
