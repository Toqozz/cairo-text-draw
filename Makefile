CC=gcc

CFLAGS=-Wall -g `pkg-config --cflags --libs cairo` `pkg-config --cflags --libs cairo-xlib` `pkg-config --cflags --libs pango` `pkg-config --cflags --libs pangocairo`
all:	draw

draw:
	$(CC) $(CFLAGS)	draw.c x.c cairo.c -o draw
clean:
	rm -f draw
