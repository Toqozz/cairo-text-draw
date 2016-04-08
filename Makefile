CC=gcc

CFLAGS=-Wall -g `pkg-config --cflags --libs cairo` `pkg-config --cflags --libs cairo-xlib`
all:	draw

draw:
	$(CC) $(CFLAGS)	draw.c x.c	-o draw
clean:
	rm -f draw
