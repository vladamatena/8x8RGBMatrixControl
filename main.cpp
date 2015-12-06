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
uint8_t mt[3][8][8];

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
	writeLatch(mt[0][intensity][row]);
	writeLatch(mt[1][intensity][row]);
	writeLatch(mt[2][intensity][row]);
	
	// Set current row
	PORTD = 0;
	srLatch();
	PORTD = 1 << row;
		
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
	TCCR1A = 0b0000'0101; // Clear Timer on Compare Match (CTC) mode
	// ICNC1 ICES1 – WGM13 WGM12 CS12 CS11 CS10 
	TCCR1B = 0b0001'0011; // clock source CLK/1024, start timer
	TIFR1 |= 1 << 6; // clear interrupt flag
	TIMSK1 |= 1 << OCIE1A; // TC0 compare match A interrupt enable
	sei();
	
	for(int i = 0 ; i <= MAX_INTENSITY; i ++) {
		for(int r = 0 ; r <= MAX_ROW; r ++) {
			mt[0][i][r] = 0b11111111;
			mt[1][i][r] = 0b11111111;
			mt[2][i][r] = 0b11111111;
		}
	}
	
	uint8_t ROW2INT[] = {0, 1, 2, 3, 4, 5, 6, 7};
	
	for(int r = 0; r < 8; ++r) {
		const uint8_t intensity = ROW2INT[r];
		for(int i = 0; i < intensity; ++i) {
			mt[2][i][r] = ~(1 << r);
		}
	}

	while(true);
}
