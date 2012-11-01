all: hwnotify


hwnotify: hwnotify.o
	gcc -o hwnotify hwnotify.o -lftdi

hwnotify.o: hwnotify.c
	gcc -c -g -O0 hwnotify.c

clean:
	rm -f hwnotify.o hwnotify