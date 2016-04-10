#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "x.h"
#include "cairo.h"

const int interval = 16.5; //60fps == 16.5

// Please save me, this time I cannot run.
void help()
{
    printf("basic window making and text printing.\n"
           "usage: cairo [-h | -s | -f | -m | -l | -w]\n"
           "        -h Show this help\n"
           "        -s String to print\n"
           "        -f Font to use\n"
           "        -m Distance from the left side of the window\n"
           "        -u Distance text is placed from the top of the window\n"
           "        -l Length of the window\n"
           "        -w Width of the window\n"
           );
    exit(0);
}

void parse(char *wxh, int *width, int *height)
{
    char *w;
    char *h;
    // We need something that is mutable.
    // Remember pointer position (it's being mutated).
    char *dupe = strdup(wxh);
    char *point = dupe;

    // If memeory not got.
    if (dupe == 0)
        exit(1);

    // ex. "500x10"
    w = strsep(&dupe, "x");         // w = "500", dupe = "10"
    h = strsep(&dupe, "x");         // h = "10", dupe = ""

    // Change variables 'globally' in memory. (*width and *height have memory addresses from main.).
    *width = strtol(w, NULL, 10);   // change value width is pointing to to something else.
    *height = strtol(h, NULL, 10);  // ""
    free(point);                    // finally free the pointer after EVERYTHING is done with it.
}

// TODO, think about creating the struct on the heap instead of the stack (especially for notification daemon.
struct Variables {
    char *font;
    char *string;
    char *dimensions;
    bool  italic;
    int   margin;
    int   upper;
    int   width;
    int   height;
};

const struct Variables VAR_DEFAULT = {
    .font = "Calibre",
    .string = "NULL",
    .dimensions = "300x300",
    .italic = false,
    .margin = 5,
    .upper = 5,
    .width = 300,
    .height = 20
};


int
main (int argc, char *argv[])
{
    struct Variables info = VAR_DEFAULT;

    int opt;
    while ((opt = getopt(argc, argv, "hf:m:u:d:i")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            case 'f': info.font = optarg;  break;
            case 'm': info.margin = strtol(optarg, NULL, 10); break;
            case 'u': info.upper = strtol(optarg, NULL, 10);  break;
            case 'd': info.dimensions = optarg; break;
            case 'i': info.italic = true; break;
            default: help();
        }
    }

    // Read stdin.
    int read;
    unsigned long len = 0;
    read = getline(&info.string, &len, stdin);
    if (read == -1)
        printf("No input read...\n");
    //else
        //info.string[strlen(info.string)-1] = '\0';

    // Option checking.
    if (!info.font) printf("Font is required\n");
    if (info.margin < 0) info.margin = 5;
    if (info.upper < 0) info.upper = 5;
    parse(info.dimensions, &info.width, &info.height);

    cairo_surface_t *surface;
    cairo_t *context;
    cairo_text_extents_t text;

    surface = cairo_create_x11_surface(info.width, info.height);
    context = cairo_create(surface);

    // Italic? y/n.
    cairo_select_font_face(context, info.font,
            (info.italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = interval*1000000;

    int enter = -info.width-1;
    int running;
    for (running = 1; running == 1;)
    {
        cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
        cairo_paint(context);
        cairo_set_operator(context, CAIRO_OPERATOR_OVER);

        if (enter < 0) {
            // "Animation".
            enter++;
            enter = enter/1.05;

            // TODO, is this faster?
            //cairo_xlib_surface_set_size(surface, enter+info->width, 23);

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that slides out >>.
            rounded_rectangle(enter, 0, info.width, info.height, 1, 0, context, 1,0.5,0,1);

            // Text rectangle.
            cairo_set_font_size(context, 11);
            cairo_text_extents(context, info.string, &text);

            // Make the source contain the text.
            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, enter, text.height + info.upper);
            cairo_show_text(context, info.string);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            cairo_paint(context);
            cairo_surface_flush(surface);
        }
        else {
            // Ditto.
            cairo_push_group(context);

            rounded_rectangle(0, 0, info.width, info.height, 1, 0, context, 1,0.5,0,1);

            cairo_set_font_size(context, 11);
            cairo_text_extents(context, info.string, &text);

            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, enter, text.height + info.upper);
            cairo_show_text(context, info.string);

            cairo_pop_group_to_source(context);

            cairo_paint(context);
            cairo_surface_flush(surface);

            // "Animation".
            enter++;
        }

        switch (check_x_event(surface, 0))
        {
            case -3053:
                fprintf(stderr, "exposed\n");
                break;
            case 0xff53:    // right cursor
                fprintf(stderr, "right cursor pressed\n");
                break;
            case 0xff51:    // left cursor
                fprintf(stderr, "left cursor pressed\n"); //wtf is a cursor compared to a mouse button.
                break;
            case 0xff1b:    // esc
            case -1:        // left mouse button
                fprintf(stderr, "left mouse button\n");
                running = 0;
                break;
        }

        nanosleep(&req, &req);
    }

    cairo_destroy(context);
    destroy(surface);
    free(info.string);

    return 0;
}
