CC=gcc

CFLAGS=-Wall -g `pkg-config --cflags --libs cairo` `pkg-config --cflags --libs cairo-xlib`
all:	cairo

cairo:
	$(CC) $(CFLAGS)	cairo.c x.c	-o cairo
clean:
	rm -f cairo
