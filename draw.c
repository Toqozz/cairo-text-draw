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

//#include <pthread.h>

#include "x.h"
#include "cairo.h"

const int interval = 33; //60fps == 16.5

struct Variables {
    char *font;
    //char *string;
    bool  italic;
    int   margin;
    int   upper;
    int   x;
    int   y;
    int   width;
    int   height;
};

struct MessageInfo {
    char *string;
    int   x;
    int   y;
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
*var_create(char *font, bool italic, int margin,
            int upper, int width, int height)
{
    struct Variables *info = malloc(sizeof(struct Variables));
    assert(info != NULL);

    info->font = font;
    //info->string = strdup(string);
    info->italic = italic;
    info->margin = margin;
    info->upper = upper;
    info->x = 300;
    info->y = 300;
    info->width = width;
    info->height = height;

    return info;
}

// Create messages on the stack.
struct MessageInfo
message_create(char *string, int x, int y)
{
    struct MessageInfo message;

    message.string = string;
    message.x = x;
    message.y = y;

    return message;
}

// Can be reused for both structs.
void
var_destroy(struct Variables *destroy)
{
    assert(destroy != NULL);

    //free(destroy->string);
    free(destroy);
}

void
runner(struct Variables *info, char *strings[])
{
    cairo_surface_t *surface;
    cairo_t *context;
    PangoRectangle extents;
    PangoLayout *layout;
    PangoFontDescription *desc;

    // TODO, implement this.
    //cairo_xlib_surface_set_size(surface, enter+info->width, 23);
    // TODO: REPLACE with detection:
    // if new message, message create on the end of the array.

    // Surface for drawing on, layout for putting the font on.
    surface = cairo_create_x11_surface(info->width, info->height*4+40);
    context = cairo_create(surface);
    layout = pango_cairo_create_layout (context);

    // Font selection with pango.
    desc = pango_font_description_from_string(info->font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc); // be free my child.

    // Interval = 33 = 30fps.
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = interval*1000000;

    struct MessageInfo messages[10];
    int i;
    for (i = 0; i < 4; i++)
        messages[i] = message_create (strings[i], -info->width-1, i*30);
        //printf("messages%d: string:%s, x:%d, y:%d\n", i, strings[i], messages[i].x, messages[i].y);

    int running;
    for (running = 1; running == 1;)
    {
        // Clear the surface.
        cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
        cairo_paint(context);
        cairo_set_operator(context, CAIRO_OPERATOR_OVER);

        cairo_push_group(context);

        for (i = 0; i < 3; i++)
        {
            if (messages[i].x < 0) {

                messages[i].x++;
                messages[i].x = messages[i].x/1.05;

                // Rectangle for each message.
                rounded_rectangle(messages[i].x, messages[i].y, info->width, info->height, 1, 0, context, 1,0.5,0,1);

                // Allow markup on the string.
                pango_layout_set_markup(layout, messages[i].string, -1);

                // Pixel extents are much better for this purpose.
                pango_layout_get_pixel_extents(layout, &extents, NULL);

                // Make the source contain the text.
                cairo_set_source_rgba(context, 0,0,0,1);
                cairo_move_to(context, messages[i].x, messages[i].y + info->upper);
                pango_cairo_show_layout(context, layout);

                // A margin for the thing, TODO: call this option: margin.
                cairo_set_source_rgba(context, 1,0.5,0,1);
                cairo_rectangle(context, 0, messages[i].y, 10, info->height);
                cairo_fill(context);
            }
            else {
                rounded_rectangle(0, messages[i].y, info->width, info->height, 1, 0, context, 1,0.5,0,1);

                pango_layout_set_markup(layout, messages[i].string, -1);
                pango_layout_get_pixel_extents(layout, &extents, NULL);

                cairo_set_source_rgba(context, 0,0,0,1);
                cairo_move_to(context, messages[i].x - extents.width, messages[i].y + info->upper);
                pango_cairo_show_layout(context, layout);

                cairo_set_source_rgba(context, 1,0.5,0,1);
                cairo_rectangle(context, 0, messages[i].y, 10, info->height);
                cairo_fill(context);

                // "Animation".
                messages[i].x++;
            }
        }


        // Pop the group to source.
        cairo_pop_group_to_source(context);

        // Paint the source.
        cairo_paint(context);
        cairo_surface_flush(surface);


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
    int  margin = 0, number = 0,
         upper = 0, width = 0,
         height = 0;
    bool italic = false;
    char *font;
    char *dimensions;


    int  opt;
    while ((opt = getopt(argc, argv, "hf:s:m:n:u:d:i")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            case 'f': font = optarg;  break;
            case 'm': margin = strtol(optarg, NULL, 10); break;
            case 'n': number = strtol(optarg, NULL, 10); break;
            case 'u': upper = strtol(optarg, NULL, 10);  break;
            case 'd': dimensions = optarg; break;
            case 'i': italic = true; break;
            default: help();
        }
    }

    // Initialise to NULL and read stdin.  If the char is NULL, getline will do memory allocation for us.
    char *strings[number]; // getline will allocate memory for us if the pointer is NULL.
    int i;
    int status;
    unsigned long len;
    for (i = 0; i < number; i++) {
        strings[i] = NULL;
        status = getline(&strings[i], &len, stdin);

        //printf("No input read...\n");
        if (status == -1) exit(1);
        else strings[i][strlen(strings[i])-1] = '\0';
    }

    // Option checking.
    if (!font) printf("Font is required\n");
    if (!dimensions) dimensions = "300x300";
    if (margin < 0) margin = 5;
    if (upper < 0) upper = 5;
    parse(dimensions, &width, &height);

    // Create info on the heap.
    struct Variables *info = var_create(font, italic, margin, upper, width, height);

    // Run until done.
    runner(info, strings);

    // Done with strings -- program ending.
    for (i = 0; i < number; i++)
        free(strings[i]);

    return(0);

}
