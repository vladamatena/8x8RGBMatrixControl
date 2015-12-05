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

int main (void) {
	blink();
}
