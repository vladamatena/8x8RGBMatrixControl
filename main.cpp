#define F_CPU 1000000UL
#define F_OSC 1000000

#include <avr/io.h>
#include <util/delay.h>

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

void srSetup() {
	DDRC |= 0b00000111;
	PORTC = 0b00000000;
}

void srClock() {
	PORTC ^= 0b00000001;
	PORTC ^= 0b00000001;
}

void srData(bool data) {
	if(data) {
		PORTC |= 0b00000010;
	} else {
		PORTC &= ~0b00000010;
	}
	_delay_ms(1);
}

void srLatch() {
	PORTC ^= 0b00000100;
	PORTC ^= 0b00000100;
}

void writeLatch(uint8_t data) {
	for(int i = 0; i < 8; ++i) {
		srData(data & (1 << i));
		srClock();
	}
	
	srLatch();
}

// R 0b11000000
// G 0b00110000
// B 0b00001100
volatile uint8_t mt[4][8][8];

int main (void) {
	DDRC = 0b00000111;
	
	DDRD = 0b11111111;
	PORTD = 0b11111111;
	//blink();
	
	srSetup();
	
/*	uint8_t data = 0;
	while(true) {
		writeLatch(0b00000000);
		_delay_ms(100);
	}*/

	while(true) {
		for(int r = 0; r < 8; ++r) {
			PORTD = 1 << r;
			
			for(int c = 0; c < 255; c++) {
				writeLatch(~c);
				_delay_ms(25);
			}
		}
	
	}
}
