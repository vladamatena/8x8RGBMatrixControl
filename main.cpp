#define F_CPU 8000000UL
#define F_OSC 8000000

#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define BAUD 9600
#include <util/setbaud.h>




#define NEGPORT PORTB
#define NEGDDR DDRB

inline void srSetup() {
	DDRC |= 0b00000111;
	PORTC = 0b00000000;
}

inline void srClock() {
	PORTC ^= 0b00000010;
	PORTC ^= 0b00000010;
}

inline void srLatch() {
	PORTC ^= 0b00000001;
	PORTC ^= 0b00000001;
}

inline void writeLatch(uint8_t data) {
	for(int i = 0; i < 8; ++i) {
		PORTC &= ~0b00000100;
		if(data & 0b00000001) {
			PORTC |= 0b00000100;
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

uint16_t fb[ROWS][COLLUMNS]; // 0b----RRRRGGGGBBBB



void uart_init(void) {
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0); // Enable RX and TX and interrupt
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data
}

void uart_putchar(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0); // Wait until data register empty. 
    UDR0 = c;
}

void uart_puts(const char *str) {
	while(*str) {
		uart_putchar(*str++);
	}
}

char uart_getchar(void) {
    loop_until_bit_is_set(UCSR0A, RXC0); /* Wait until data exists. */
    return UDR0;
}

uint8_t reverse(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

// Render frambuffer to mt data structure
void render() {
	// Erase
	for(int i = 0 ; i < INTENSITIES; i++) {
		for(int r = 0 ; r < ROWS; r++) {
			mt[0][i * ROWS + r] = 0b11111111;
			mt[1][i * ROWS + r] = 0b11111111;
			mt[2][i * ROWS + r] = 0b11111111;
		}
	}
	
	// Set new data
	for(int r = 0; r < 8; r++) {
		for(int c = 0; c < 8; c++) {
			const uint8_t red = (fb[r][c] & 0b0000111100000000) >> 8;
			const uint8_t green = (fb[r][c] & 0b0000000011110000) >> 4;
			const uint8_t blue = fb[r][c] & 0b0000000000001111;
			
			for(int i = 0; i < INTENSITIES;i++) {
				if(red > i) mt[0][i * ROWS + r] &= reverse(~(1 << c));
				if(green > i) mt[1][i * ROWS + r] &= ~(1 << c);
				if(blue > i) mt[2][i * ROWS + r] &= ~(1 << c);
			}
		}
	}
}

uint8_t pos = 0;
bool dirty = false;
// Serial coomunication interrupt
ISR(USART_RX_vect) {
	union {
		uint16_t val;
		uint8_t bytes[2];
	} data;
	
	data.bytes[0] = UDR0;
	data.bytes[1] = uart_getchar();
	
	// reset - new image
	if(data.val == 0b1000000000000000) {
		pos = 0;
		return;
	}
	
	// render - end of image
	if(data.val == 0b0100000000000000) {
		dirty = true;
		return;
	}
	
	((uint16_t*)fb)[pos++ % 64] = data.val;
}

/**
 * Display output for this period of Timer
 */
uint8_t ir = 0;
inline void serviceScreen() {
	writeLatch(mt[0][ir]);
	writeLatch(mt[1][ir]);
	writeLatch(mt[2][ir]);
	
	// Set current row
	NEGPORT = 0;
	srLatch();
	NEGPORT = 1 << ir % ROWS;
	
	ir++;
	if(ir > (INTENSITIES * ROWS - 1)) {
		ir = 0;
	}
}

void clockTo8MHz() {
	// 8Mhz - no prescaller system clock
	asm volatile (
	"st Z,%1" "\n\t"
	"st Z,%2"
	: :
	"z" (&CLKPR),
	"r" ((uint8_t) (1<<CLKPCE)),
	"r" ((uint8_t) 0)  // new CLKPR value
	);
}


int main (void) {
	// First disable output
	NEGPORT = 0b00000000;
	NEGDDR = 0b11111111;
	
	_delay_ms(100);
	
	clockTo8MHz();
	
	DDRC = 0b00010111;
	
	
	// Clear screen
	srSetup();
	writeLatch(0b11111111);
	writeLatch(0b11111111);
	writeLatch(0b11111111);
	
	// Demo pattern
	for(int r = 0; r < 8; r++) {
		for(int c = 0; c < 8; c++) {
			fb[r][c] = (((8-c)+r & 0b1111) << 8) | ((r*2 & 0b1111) << 4) | (c*2 & 0b1111);
		}
	}
	render();
	
	
	uart_init();
	sei();
	
	while(true) {
		serviceScreen();
		
		if(dirty) {
			render();
			dirty = false;
		}
	}
}
