program=hello

hello: main.hex

main.elf: main.cpp
	avr-gcc -Os -std=c++1y -mmcu=atmega328p -o $@ $<

main.hex: main.elf
	avr-objcopy -R .eeprom -O ihex $< $@

clean: main.hex main.elf
	rm main.hex
	rm main.elf

flash: main.hex
	avrdude -u -c usbasp -p atmega328p -U flash:w:main.hex:i
