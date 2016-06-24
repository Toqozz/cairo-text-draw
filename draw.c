#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <pango/pangocairo.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "x.h"
#include "cairo.h"

const int interval = 33; //60fps == 16.5

struct Variables {
    char *font;
    char *string;
    bool  italic;
    int   margin;
    int   upper;
    int   x;
    int   y;
    int   width;
    int   height;
};

// Please save me, this time I cannot run.
void
help()
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

void
parse(char *wxh, int *width, int *height)
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

// Create a struct on the heap.
struct Variables 
*var_create(char *font, char *string, bool italic, int margin,
                             int upper, int width, int height)
{
    struct Variables *info = malloc(sizeof(struct Variables));
    assert(info != NULL);

    info->font = strdup(font);
    info->string = strdup(string);
    info->italic = italic;
    info->margin = margin;
    info->upper = upper;
    info->x = 300;
    info->y = 300;
    info->width = width;
    info->height = height;

    return info;
}

void
var_destroy(struct Variables *destroy)
{
    assert(destroy != NULL);

    free(destroy->font);
    free(destroy->string);
    free(destroy);
}

void
runner(struct Variables *info)
{
    cairo_surface_t *surface;
    cairo_t *context;
    PangoRectangle extents;
    PangoLayout *layout;
    PangoFontDescription *desc;

    // Surface for drawing on, layout for putting the font on.
    surface = cairo_create_x11_surface(info->width, info->height);
    context = cairo_create(surface);
    layout = pango_cairo_create_layout (context);

    // Font selection with pango.
    // Supports pango markup.
    pango_layout_set_markup(layout, info->string, -1);
    desc = pango_font_description_from_string(info->font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc); // be free my child.

    // Text extents, coolio.
    // Pixel extents are much better for this purpose.
    pango_layout_get_pixel_extents(layout, &extents, NULL);
    //printf("extentw: %d\n", extents.width);
    //printf("extenth: %d\n", extents.height);

    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = interval*1000000;

    int enter = -info->width-1;
    int running;
    for (running = 1; running == 1;)
    {
        cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
        cairo_paint(context);
        cairo_set_operator(context, CAIRO_OPERATOR_OVER);

        // TODO adapt to multiple messages.
        // how can i move them all at once?
        // -threaded
        // -single(will look weird)
        //      maybe it wont! push group is life saving.
        //      !

        if (enter < 0) {
            // "Animation".
            enter++;
            enter = enter/1.05;

            // TODO, is this faster?
            //cairo_xlib_surface_set_size(surface, enter+info->width, 23);

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that slides out >>.
            rounded_rectangle(enter, 0, info->width, info->height, 1, 0, context, 1,0.5,0,1);

            // Make the source contain the text.
            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, enter - extents.width, extents.height/2 + info->upper);
            pango_cairo_show_layout(context, layout);

            // A margin for the thing, TODO: call this option: margin.
            cairo_set_source_rgba(context, 1,0.5,0,1);
            cairo_rectangle(context, 0, 0, 10, info->height);
            cairo_fill(context);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            cairo_paint(context);
            cairo_surface_flush(surface);
        }
        else {
            // Ditto.
            cairo_push_group(context);

            rounded_rectangle(0, 0, info->width, info->height, 1, 0, context, 1,0.5,0,1);

            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, enter - extents.width, info->upper);
            pango_cairo_show_layout(context, layout);

            cairo_set_source_rgba(context, 1,0.5,0,1);
            cairo_rectangle(context, 0, 0, 10, info->height);
            cairo_fill(context);

            cairo_pop_group_to_source(context);

            cairo_paint(context);
            cairo_surface_flush(surface);

            // "Animation".
            enter++;
        }

        // if new_event() {
        //      make new rectangle above previous one.
        //          how?
        //          use for loop in rounded_rectangle clause?
        //          .. messages are in an array
        //              how to keep positions intact?
        //                  have to record them this way.
        //
        //          would you use structs for tihs? yes, on the stack i think
        //          what about rectangle? it comes out before the text -- they are separate.
        //          well, the current method is used to that.
        //          for i in messages:
        //              moveto (messagepos)
        //              print (message)
        //              set_new_position.
        //      record position of rectangle.
        //
        // }

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
    // Destroy once we are done.
    var_destroy(info);
}

int
main (int argc, char *argv[])
{

    // Option initialization.
    int  margin = 0, upper = 0,
         width = 0, height = 0;
    bool italic = false;
    char *font;
    char *dimensions;
    char *string = NULL; // getline will allocate memory for us if the pointer is NULL.

    int  opt;
    while ((opt = getopt(argc, argv, "hf:s:m:u:d:i")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            case 'f': font = optarg;  break;
            case 'm': margin = strtol(optarg, NULL, 10); break;
            case 'u': upper = strtol(optarg, NULL, 10);  break;
            case 'd': dimensions = optarg; break;
            case 'i': italic = true; break;
            default: help();
        }
    }

    // Read stdin.
    int read;
    unsigned long len;
    read = getline(&string, &len, stdin);
    if (read == -1) {
        printf("No input read...\n");
        exit(1);
    }
    else
        string[strlen(string)-1] = '\0';

    // Option checking.
    if (!font) printf("Font is required\n");
    if (!dimensions) dimensions = "500x20";
    if (margin < 0) margin = 5;
    if (upper < 0) upper = 5;
    parse(dimensions, &width, &height);

    // Create info on the heap.
    // TODO, parse x, y. (position on window).
    // Do you mean let the user set  where theyre going? already done.. kind of.
    struct Variables *info = var_create(font, string, italic, margin, upper, width, height);
    // Done with string -- it's info's job now.
    free(string);

    // Run until we are done.
    runner(info);

    return(0);

}
