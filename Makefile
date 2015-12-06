program=control

control: main.hex

CPPFLAGS=-Ofast -std=c++1y -mmcu=atmega328p -fno-exceptions

main.elf: main.cpp
	avr-gcc ${CPPFLAGS} -o $@ $<

%.hex: %.elf
	avr-objcopy -R .eeprom -O ihex $< $@

clean:
	rm *.hex
	rm *.elf

flash: main.hex
	avrdude -u -c usbasp -p atmega328p -U flash:w:main.hex:i
