all: hwnotify-eeprom


hwnotify-eeprom: hwnotify-eeprom.o
	gcc -o hwnotify-eeprom hwnotify-eeprom.o -lftdi

hwnotify-eeprom.o: hwnotify-eeprom.c
	gcc -c -g -O0 hwnotify-eeprom.c

clean:
	rm -f hwnotify-eeprom.o hwnotify-eeprom