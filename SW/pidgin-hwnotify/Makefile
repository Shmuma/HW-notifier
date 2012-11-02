# hwnotify Makefile
# Max Lapan <max.lapan@gmail.com>

GTK_PIDGIN_INCLUDES= `pkg-config --cflags gtk+-2.0 pidgin`

GTK_PREFIX=/usr/local
GTK_PREFIX2=/usr

CFLAGS= -O0 -Wall -fpic -g
LDFLAGS= -shared

INCLUDES = \
      -I$(GTK_PREFIX)/include \
      -I$(GTK_PREFIX)/include/gtk-2.0 \
      -I$(GTK_PREFIX)/include/glib-2.0 \
      -I$(GTK_PREFIX)/include/pango-1.0 \
      -I$(GTK_PREFIX)/include/atk-1.0 \
      -I$(GTK_PREFIX)/lib/glib-2.0/include \
      -I$(GTK_PREFIX)/lib/gtk-2.0/include \
      -I$(GTK_PREFIX2)/include \
      -I$(GTK_PREFIX2)/include/gtk-2.0 \
      -I$(GTK_PREFIX2)/include/glib-2.0 \
      -I$(GTK_PREFIX2)/include/pango-1.0 \
      -I$(GTK_PREFIX2)/include/atk-1.0 \
      -I$(GTK_PREFIX2)/lib/glib-2.0/include \
      -I$(GTK_PREFIX2)/lib/gtk-2.0/include \
      $(GTK_PIDGIN_INCLUDES)

hwnotify.so: hwnotify.c
	gcc hwnotify.c $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o hwnotify.so

install: hwnotify.so
	cp hwnotify.so ~/.purple/plugins/

uninstall:
	rm -f ~/.purple/plugins/hwnotify.so

clean:
	rm -f hwnotify.so

