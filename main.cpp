#define F_CPU 8000000UL
#define F_OSC 8000000

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



const uint8_t INTENSITIES = 16;
const uint8_t MAX_INTENSITY = INTENSITIES - 1;

const uint8_t ROWS = 8;
const uint8_t MAX_ROW = ROWS - 1;

const uint8_t COLLUMNS = 8;
const uint8_t MAX_COLLUMN = COLLUMNS - 1;

/**
 * Framebuffer
 * 
 * mt[COLOR_CNANNEL][INTENSITY_PLANE * 8 + row]
 * 
 * Framebuffer is cyclicaly rendered every time one row wit one intensity plane.
 * Intensity is defined by number of intensity planes containing 1 in the place
 * representing particular LED.
 */
uint8_t mt[3][INTENSITIES * ROWS];



/**
 * Timer interrupt
 * 
 * Display output for this period of Timer
 * 
 */
uint8_t row = 0;
uint8_t intensity = 0;
uint8_t ir = 0;
SIGNAL(TIMER1_COMPA_vect) {
	writeLatch(mt[0][ir]);
	writeLatch(mt[1][ir]);
	writeLatch(mt[2][ir]);
	
	// Set current row
	PORTD = 0;
	srLatch();
	PORTD = 1 << ir % ROWS;
	
	ir++;
	//ir &= 0b00111111;
	if(ir > (INTENSITIES * ROWS - 1))
		ir = 0;
}

int main (void) {
	// First disable output
	PORTD = 0b00000000;
	DDRD = 0b11111111;
	
	
	// 8Mhz - no prescaller system clock
	asm volatile (
	"st Z,%1" "\n\t"
	"st Z,%2"
	: :
	"z" (&CLKPR),
	"r" ((uint8_t) (1<<CLKPCE)),
	"r" ((uint8_t) 0)  // new CLKPR value
	);
	
	DDRC = 0b00010111;
	
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
	OCR1A  = 10; // Number to count up to
	// COM1A1 COM1A0 COM1B1 COM1B0 – – WGM11 WGM10
	TCCR1A = 0b0000'0101; // Clear Timer on Compare Match (CTC) mode
	// ICNC1 ICES1 – WGM13 WGM12 CS12 CS11 CS10 
	TCCR1B = 0b0001'0011; // clock source CLK/1024, start timer
	TIFR1 |= 1 << 6; // clear interrupt flag
	TIMSK1 |= 1 << OCIE1A; // TC0 compare match A interrupt enable
	sei();
	
	uint8_t step = 0;
	while(true) {
		// Erase
		for(int i = 0 ; i < INTENSITIES; i++) {
			for(int r = 0 ; r < ROWS; r++) {
				mt[0][i * ROWS + r] = 0b11111111;
				mt[1][i * ROWS + r] = 0b11111111;
				mt[2][i * ROWS + r] = 0b11111111;
			}
		}
		
	/*	
		// Diagonal
		for(int r = 0; r < ROWS; ++r) {
			const uint8_t intensity = (r + step) % INTENSITIES;
			for(int i = 0; i < intensity; ++i) {
				mt[2][i * ROWS + r] = ~(1 << r);
			}
		}*/
		
		
		// Pattern
		for(int i = 0 ; i <= MAX_INTENSITY; i++) {
			for(int r = 0 ; r <= MAX_ROW; r++) {
				for(int c = 0; c <= MAX_COLLUMN; c++) {
					if(i <= (c + r*(step / 32) + step) % INTENSITIES) {
						mt[0][i * ROWS + r] &= ~(1 << c);
						mt[1][i * ROWS + r] &= ~(1 << c);
						mt[2][i * ROWS + r] &= ~(1 << c);
					}
				}
			}
		}
		
		
		
		_delay_ms(50);
		step++;
		
		// Liveness blink
		PORTC ^= 0b00010000;
	}
}
