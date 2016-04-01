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

static void x_set_wm(Window win, Display *dsp)
{
    Atom  property[2];

    char *title = "yarn";
    Atom _net_wm_title = XInternAtom(dsp, "_NET_WM_NAME", false);
    // Set or read a window's WM_NAME property.
    XStoreName(dsp, win, title);
    XChangeProperty(dsp, win, _net_wm_title, XInternAtom(dsp, "UTF8_STRING", false), 8, PropModeReplace, (unsigned char *) title, strlen(title));

    char *class = "yarn";
    XClassHint classhint = { class, "yarn" };
    XSetClassHint(dsp, win, &classhint);

    Atom net_wm_window_type = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE", false);
    property[0] = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", false);
    property[1] = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_UTILITY", false);

    XChangeProperty(dsp, win, net_wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) property, 2L);

    Atom net_wm_state = XInternAtom(dsp, "_NET_WM_STATE", false);
    property[0] = XInternAtom(dsp, "_NET_WM_STATE_ABOVE", false);
    XChangeProperty(dsp, win, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *) property, 1L);
    //XChangeProperty(dsp, win, net_wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &hints, 5);
}

cairo_surface_t *
cairo_create_x11_surface0(int x, int y)
{
    Display *dsp;
    Drawable da;
    int screen;
    cairo_surface_t *sfc;

    if ((dsp = XOpenDisplay(NULL)) == NULL)
        exit(1);
    screen = DefaultScreen(dsp);
    da = XCreateWindow(dsp, DefaultRootWindow(dsp),
            100,400,x,y,0,CopyFromParent,CopyFromParent,CopyFromParent,0,0);
    x_set_wm(da, dsp);
    XSelectInput(dsp, da, ButtonPressMask | KeyPressMask);
    XMapWindow(dsp, da);

    sfc = cairo_xlib_surface_create(dsp, da,
            DefaultVisual(dsp, screen), x, y);
    cairo_xlib_surface_set_size(sfc, x, y);


    return sfc;
}

int
cairo_check_event(cairo_surface_t *sfc, int block)
{
    char keybuf[8];
    KeySym key;
    XEvent e;

    for (;;)
    {
        if (block || XPending(cairo_xlib_surface_get_display(sfc)))
            XNextEvent(cairo_xlib_surface_get_display(sfc), &e);
        else
            return 0;

        switch (e.type)
        {
            case ButtonPress:
                return -e.xbutton.button;
            case KeyPress:
                XLookupString(&e.xkey, keybuf, sizeof(keybuf), &key, NULL);
                return key;
            default:
                fprintf(stderr, "Dropping unhandled XEvent.type = %d.\n", e.type);
        }
    }
}

void destroy(cairo_surface_t *sfc)
{
    Display *dsp = cairo_xlib_surface_get_display(sfc);

    cairo_surface_destroy(sfc);
    XCloseDisplay(dsp);
}

void parse(char *wxh, int *width, int *height)
{
    char *w;
    char *h;
    // We need something that is mutable.
    char *dupe = strdup(wxh);
    // If memeory got.
    if (dupe != NULL)
    {
        // ex. "500x10"
        w = strsep(&dupe, "x");         // w = "500", dupe = "10"
        h = strsep(&dupe, "x");         // h = "10", dupe = ""
        free(dupe);                     // still contains null terminator?

        // Change variables 'globally' in memory. (*width and *height have memory addresses from main.).
        *width = strtol(w, NULL, 10);   // change value width is pointing to to something else.
        *height = strtol(h, NULL, 10);  // ""
    }

}

// Modified from http://cairographics.org/samples/rounded_rectangle/
void rounded_rectangle(double x, double y,
                       double width, double height,
                       double aspect, double corner_radius,
                       cairo_t *context,
                       double r, double g,
                       double b, double a)
{
    double radius = corner_radius / aspect;
    double degrees = M_PI / 180.0;

    cairo_new_sub_path(context);
    cairo_arc(context, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(context, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(context, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(context, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(context);

    cairo_set_source_rgba(context, r, g, b, a);
    cairo_fill(context);
}

const int interval = 16.5; //60fps == 16.5

int
main (int argc, char *argv[])
{
    char *font = NULL;
    //char *string;
    char *string = NULL;
    char *dimensions;
    bool  italic = false;
    int   margin = -1;
    int   upper = -1;
    int   width = 300;
    int   height = 300;
    int   read;
    unsigned long len;

    int opt;
    while ((opt = getopt(argc, argv, "hf:m:u:d:i")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            //case 's': string = optarg; break;
            case 'f': font = optarg;  break;
            case 'm': margin = strtol(optarg, NULL, 10); break;
            case 'u': upper = strtol(optarg, NULL, 10);  break;
            case 'd': dimensions = optarg; break;
            case 'i': italic = true; break;
            default: help();
        }
    }

    // Read stdin.
    read = getline(&string, &len, stdin);
    if (-1 != read)
        puts(string);
    else
        printf("No input read...\n");

    // Option checking.
    if (!font) printf("Font is required\n");
    if (margin < 0) margin = 5;
    if (upper < 0) upper = 5;
    parse(dimensions, &width, &height);

    cairo_surface_t *surface;
    cairo_t *context;
    cairo_pattern_t *pattern;
    cairo_text_extents_t text;

    // TODO, give up on rectangles....

    surface = cairo_create_x11_surface0(width, height);
    context = cairo_create(surface);
    pattern = cairo_pattern_create_rgba(1,0.5,0,0.1);

    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = interval*1000000;

    int running;
    int i = 0;
    int enter = -width-1;
    for (running = 1; running == 1;)
    {
        if (enter < 0) {
            // "Animation".
            enter++;
            enter = enter/1.05;

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that slides out >>.
            rounded_rectangle(enter, 0, width, height, 1, 10, context, 1,0.5,0,1);

            cairo_set_source_rgba(context, 0,0,0,1);

            // Italic? y/n.
            cairo_select_font_face(context, font,
                    (italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

            // Text rectangle.
            cairo_set_font_size(context, 11);
            cairo_text_extents(context, string, &text);

            // Make the source contain the text.
            cairo_move_to(context, enter, text.height + upper);
            cairo_show_text(context, string);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            //cairo_paint_with_alpha(context, 1);
            cairo_mask(context, pattern);
            cairo_surface_flush(surface);

        }
        else {
            //cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
            //rounded_rectangle(0, 0, width, height, 1, 10, context, 1,0.5,0,1);
            //cairo_set_operator(context, CAIRO_OPERATOR_OVER);

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that stays.
            rounded_rectangle(0, 0, width, height, 1, 10, context, 1,0.5,0,1);

            cairo_set_source_rgba(context, 0,0,0,1);
            //cairo_set_operator(context, CAIRO_OPERATOR_OVER);

            // Italic? y/n.
            cairo_select_font_face(context, font,
                    (italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

            // Text rectangle.
            cairo_set_font_size(context, 11);
            cairo_text_extents(context, string, &text);

            // Make the source contain the text.
            cairo_set_source_rgba(context, 0,0,0,1);
            cairo_move_to(context, enter, text.height + upper);
            cairo_show_text(context, string);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            //cairo_paint_with_alpha(context, 1);
            cairo_mask(context, pattern);
            cairo_surface_flush(surface);

            // "Scroll".
            enter++;
        }


        switch (cairo_check_event(surface, 0))
        {
            case 0xff53:    // right cursor
                fprintf(stderr, "right cursor pressed");
                break;

            case 0xff51:    // left cursor
                fprintf(stderr, "left cursor pressed"); //wtf is a cursor compared to a mouse button.
                break;

            case 0xff1b:    // esc
            case -1:        // left mouse button
                fprintf(stderr, "left mouse button");
                running = 0;
                break;
        }
        cairo_set_source_rgba(context, 0,0,0,0);
        // Sleep for 33.3ms (30fps).
        nanosleep(&req, &req);
        i++;
    }

    cairo_destroy(context);
    destroy(surface);

    return 0;
}
