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
    int   margin;
    int   number;
    int   upper;
    int   gap;
    int   rounding;
    int   xpos;
    int   ypos;
    int   width;
    int   height;
};

struct MessageInfo {
    char *string;
    int   textx;
    int   texty;
    int   x;
    int   y;
};

// Please save me, this time I cannot run.
void
help()
{
    printf("basic window making and text printing.\n"
           "usage: cairo [ -h | -f | -m | -u | -d | -n | -g ]\n"
           "        -h Show this help\n"
           "        -f Font to use\n"
           "        -m Distance from the left side of the window\n"
           "        -u Distance text is placed from the top of the window\n"
           "        -d Dimensions (WxH+X+Y)\n"
           "        -n Number of messages\n"
           "        -g Gap between messages\n"
           );
    exit(0);
}

void
parse(char *wxh, int *xpos, int *ypos, int *width, int *height)
{
    char *x;
    char *y;
    char *w;
    char *h;

    // We need something that is mutable.
    // Remember pointer position (it's being mutated).
    char *dupe = strdup(wxh);
    char *point = dupe;

    // If memeory not got.
    assert(dupe != 0);

    // ex. "500x10+20+30"
    w = strsep(&dupe, "x");         // w = "500", dupe = "10+20+30"
    h = strsep(&dupe, "+");         // h = "10", dupe = "20+30"
    x = strsep(&dupe, "+");         // x = "20", dupe = "30"
    y = strsep(&dupe, "+");         // y = "30", dupe = ""
    //printf("%sx%s+%s+%s\n", w,h,x,y);

    // Change variables 'globally' in memory. (*width and *height have memory addresses from main.).
    *xpos = strtol(x, NULL, 10);   // change value xpos is pointing to to something else.
    *ypos = strtol(y, NULL, 10);
    *width = strtol(w, NULL, 10);
    *height = strtol(h, NULL, 10);

    // Free pointer after everytihng is done with it.
    free(point);
    free(dupe);
}

// Create a struct on the heap.
struct Variables
*var_create(char *font,
            int margin, int number, int upper,
            int gap, int rounding, int xpos, int ypos,
            int width, int height)
{
    struct Variables *info = malloc(sizeof(struct Variables));
    assert(info != NULL);

    info->font = font;
    info->margin = margin;
    info->number = number;
    info->upper = upper;
    info->gap = gap;
    info->rounding = rounding;
    info->xpos = xpos;
    info->ypos = ypos;
    info->width = width;
    info->height = height;

    return info;
}

// Create messages on the stack.
struct MessageInfo
message_create(char *string, int textx, int texty, int x, int y)
{
    struct MessageInfo message;

    message.string = string;
    message.textx = textx;
    message.texty =texty;
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

    // TODO: REPLACE with detection:
    // if new message, message create on the end of the array.

    // Surface for drawing on, layout for putting the font on.
    surface = cairo_create_x11_surface(info->xpos, info->ypos, info->width, (info->height + info->gap) * info->number);
    context = cairo_create(surface);
    layout = pango_cairo_create_layout (context);

    // Changes the size of the canvas.  Does NOT resize the window.
    //cairo_xlib_surface_set_size(surface, info->width, (info->height + info->gap) *  info->number);

    // Font selection with pango.
    desc = pango_font_description_from_string(info->font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc); // be free my child.

    // Interval = 33 = 30fps.
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = interval*1000000;

    struct MessageInfo messages[info->number];
    int i;
    for (i = 0; i < info->number; i++)
        messages[i] = message_create (strings[i], 0, 0, -info->width-1, i*(info->height + info->gap));
        //printf("messages%d: string:%s, x:%d, y:%d\n", i, strings[i], messages[i].x, messages[i].y);

    int running;
    for (running = 1; running == 1;)
    {
        // Clear the surface.
        cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
        cairo_paint(context);
        cairo_set_operator(context, CAIRO_OPERATOR_OVER);

        // New group (everything is pre-rendered and then shown at the same time).
        cairo_push_group(context);

        for (i = 0; i < info->number; i++)
        {
            // If the bar has reached the end, stop it.  Otherwise keep going.
            // if (messages + 1 < 0) : (message.x/=1.05) else : (message.x = 0).
            ++messages[i].x < 0 ? ((messages[i].x = messages[i].x/1.05)) : ((messages[i].x = 0));
            messages[i].textx++;

            // Draw each "panel".
            rounded_rectangle(messages[i].x, messages[i].y, info->width, info->height, 1, info->rounding, context, 1,0.5,0,1);

            // Allow markup on the string.
            // Pixel extents are much better for this purpose.
            pango_layout_set_markup(layout, messages[i].string, -1);
            pango_layout_get_pixel_extents(layout, &extents, NULL);

            // Push the text to the soure.
            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, messages[i].textx - extents.width, messages[i].y + info->upper);
            pango_cairo_show_layout(context, layout);

            // Draw over the text with a margin.
            cairo_set_source_rgba(context, 1,0.5,0,1);
            cairo_rectangle(context, 0, messages[i].y, info->margin, info->height);
            cairo_fill(context);

            // Kind of cool.
            //cairo_translate(context, info->width/2.0, info->height/2.0);
            //cairo_rotate(context, 1);
            //
            //cairo_scale(context, -1, 1);
        }


        // Pop the group.
        cairo_pop_group_to_source(context);

        // Paint it.
        cairo_paint(context);
        cairo_surface_flush(surface);

        // Clue in for x events (allows to check for hotkeys, stuff like that).
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

        // Finally sleep ("animation").
        nanosleep(&req, &req);
    }

    // Destroy once done.
    cairo_destroy(context);
    destroy(surface);
    var_destroy(info);
}

int
main (int argc, char *argv[])
{
    // Option initialization.
    int  margin = 0, number = 0,
         upper = 0, width = 0,
         xpos = 0, ypos = 0,
         height = 0, gap = 0,
         rounding = 0;
    char *font;
    char *dimensions;


    int  opt;
    while ((opt = getopt(argc, argv, "hf:m:n:u:g:r:d:")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            case 'f': font = optarg;  break;
            case 'm': margin = strtol(optarg, NULL, 10); break;
            case 'n': number = strtol(optarg, NULL, 10); break;
            case 'u': upper = strtol(optarg, NULL, 10);  break;
            case 'g': gap = strtol(optarg, NULL, 10); break;
            case 'r': rounding = strtol(optarg, NULL, 10); break;
            case 'd': dimensions = optarg; break;
            default: help();
        }
    }


    // Initialise to NULL and read stdin.  If the char is NULL, getline will do memory allocation automatically.
    char *strings[number];
    int i;
    int status;
    unsigned long len;
    for (i = 0; i < number; i++) {
        strings[i] = NULL;
        status = getline(&strings[i], &len, stdin);

        assert(status != -1);
        strings[i][strlen(strings[i])-1] = '\0';
    }

    // Option checking.
    if (!font) printf("Font is required\n");
    if (!dimensions) dimensions = "300x300";
    if (margin < 0) margin = 5;
    if (rounding < 0) rounding = 0;
    parse(dimensions, &xpos, &ypos, &width, &height);

    // Create info on the heap.
    struct Variables *info = var_create(font, margin, number, upper, gap, rounding, xpos, ypos, width, height);

    // Run until done.
    runner(info, strings);

    // Done with strings -- program ending.
    for (i = 0; i < number; i++)
        free(strings[i]);

    return(0);

}
