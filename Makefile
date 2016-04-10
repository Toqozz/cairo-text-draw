CC=gcc

CFLAGS=-Wall -g `pkg-config --cflags --libs cairo` `pkg-config --cflags --libs cairo-xlib`
all:	draw

draw:
	$(CC) $(CFLAGS)	draw.c x.c cairo.c	-o draw
clean:
	rm -f draw
