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

// Apply atoms to window.
static void 
x_set_wm(Window win, Display *dsp)
{
    Atom property[3];  // Change 2 things at once, (parent + 2 children).

    // Set window's WM_NAME property.
    // char *title = "yarn"; -- only used twice.
    XStoreName(dsp, win, "yarn");
    // No children.
    property[2] = XInternAtom(dsp, "_NET_WM_NAME", false); // Get WM_NAME atom and store it in _net_wm_title.
    XChangeProperty(dsp, win, property[2], XInternAtom(dsp, "UTF8_STRING", false), 8, PropModeReplace, (unsigned char *) "yarn", 4);

    // Set window's class.
    // char *class = "yarn"; -- only used once.
    XClassHint classhint = { "yarn", "yarn" };
    XSetClassHint(dsp, win, &classhint);

    // Parent.
    property[2] = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE", false);   // Let WM know type.
    // Children.
    property[0] = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", false);
    property[1] = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_UTILITY", false);
    // Reach for 2 longs, (2L).
    XChangeProperty(dsp, win, property[2], XA_ATOM, 32, PropModeReplace, (unsigned char *) property, 2L);

    // Parent.
    property[2] = XInternAtom(dsp, "_NET_WM_STATE", false);   // Let WM know state.
    // Child.
    property[0] = XInternAtom(dsp, "_NET_WM_STATE_ABOVE", false);
    // Reach for 1 long, (1L).
    XChangeProperty(dsp, win, property[2], XA_ATOM, 32, PropModeReplace, (unsigned char *) property, 1L);
}

// Map window and return surface for that window.
cairo_surface_t *
cairo_create_x11_surface(int x, int y)
{
    Display *display;
    Drawable drawable;
    int screen;   // Screen #.
    cairo_surface_t *surface;

    // Error if no open..
    if ((display = XOpenDisplay(NULL)) == NULL)   // Set display though.
        exit(1);

    screen = DefaultScreen(display);   // Use primary display.
    XVisualInfo vinfo;
    // Match the display settings.
    XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo);

    XSetWindowAttributes attr;
    // We need all 3 of these attributes, or BadMatch: http://stackoverflow.com/questions/3645632/how-to-create-a-window-with-a-bit-depth-of-32
    attr.colormap = XCreateColormap(display, DefaultRootWindow(display), vinfo.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0;

    drawable = XCreateWindow(display, DefaultRootWindow(display),  // Returns a window (a drawable place).
            100,400, // Position on screen.
            x,y,     // Width, Height.
            0,       // Border width.
            vinfo.depth, InputOutput, vinfo.visual,   // Depth, Class, Visual type.
            CWColormap | CWBorderPixel | CWBackPixel, // Overwritten attributes.
            &attr);

    // Apply the Atoms to the new window.
    // Request that the X server report these events.
    x_set_wm(drawable, display);
    XSelectInput(display, drawable, ExposureMask | ButtonPressMask | KeyPressMask);
    XMapWindow(display, drawable);

    // Finally create a surface from the window.
    surface = cairo_xlib_surface_create(display, drawable,
            vinfo.visual, x, y);
    cairo_xlib_surface_set_size(surface, x, y);

    return surface;
}

void
x_resize_window(Display *dsp, Window win, int x, int y)
{
    XWindowChanges values;

    values.width = x;
    values.height = y;

    XConfigureWindow(dsp, win, CWWidth | CWHeight, &values);
}

// Respond properly to events.
int
cairo_check_event(cairo_surface_t *sfc, int block)
{
    char keybuf[8];
    XEvent event;
    KeySym key;

    for (;;)
    {
        // If there is nothing in the event cue, XNextEvent will block the program.
        // If we want to block, then we can.
        if (block || XPending(cairo_xlib_surface_get_display(sfc)))
            XNextEvent(cairo_xlib_surface_get_display(sfc), &event);
        else
            return 0;

        // Check it out.
        switch (event.type)
        {
            // This event shouldn't really happen that much (we should always be exposed).
            case Expose:
                return -3053; // 3xp053
            // -event.xbutton.button -- less likely to be taken?
            case ButtonPress:
                fprintf(stderr, "the button is: %d\n", -event.xbutton.button);
                return -event.xbutton.button;
            // Don't get key press events as a utility window, useless.
            case KeyPress:
                return event.xkey.keycode;
            // Fuck this event. https://lists.cairographics.org/archives/cairo/2014-August/025534.html
            case 65:
                break;
            // We don't know the event, drop.
            default:
                fprintf(stderr, "Dropping unhandled XEvent.type = %d.\n", event.type);
                break;
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

struct Variables {
    char *font;
    char *string;
    char *dimensions;
    char *text;
    bool  italic;
    int   margin;
    int   upper;
    int   width;
    int   height;
};

const struct Variables VAR_DEFAULT = {
    .dimensions = "300x300", .italic = false, .margin = 5, .upper = 5
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
    unsigned long len;
    read = getline(&info.string, &len, stdin);
    if (read == -1)
        printf("No input read...\n");

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
            //x_resize_window(cairo_xlib_surface_get_display(surface), cairo_xlib_surface_get_drawable(surface), enter+info->width, 23);
            //cairo_xlib_surface_set_size(surface, enter+info->width, 23);

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that slides out >>.
            rounded_rectangle(enter, 0, info.width, info.height, 1, 0, context, 1,0.5,0,1);

            cairo_set_source_rgba(context, 0,0,0,1);

            // TODO, move outside of loop.
            // Italic? y/n.
            cairo_select_font_face(context, info.font,
                    (info.italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

            // Text rectangle.
            cairo_set_font_size(context, 11);
            cairo_text_extents(context, info.string, &text);

            // Make the source contain the text.
            cairo_move_to(context, enter, text.height + info.upper);
            cairo_show_text(context, info.string);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            cairo_paint(context);
            cairo_surface_flush(surface);
        }
        else {
            // TODO, is this faster?
            //x_resize_window(cairo_xlib_surface_get_display(surface), cairo_xlib_surface_get_drawable(surface), enter+info->width, 23);
            //cairo_xlib_surface_set_size(surface, enter+info->width, 23);

            // Create a new push group.
            cairo_push_group(context);

            // Rounded rectangle that slides out >>.
            rounded_rectangle(0, 0, info.width, info.height, 1, 0, context, 1,0.5,0,1);

            cairo_set_source_rgba(context, 0,0,0,1);

            // TODO, move outside of loop.
            // Italic? y/n.
            cairo_select_font_face(context, info.font,
                    (info.italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

            // Text rectangle.
            cairo_set_font_size(context, 11);
            cairo_text_extents(context, info.string, &text);

            // Make the source contain the text.
            cairo_move_to(context, enter, text.height + info.upper);
            cairo_show_text(context, info.string);

            // Pop the group to source.
            cairo_pop_group_to_source(context);

            // Paint the source.
            cairo_paint(context);
            cairo_surface_flush(surface);

            // "Animation".
            enter++;
        }

        switch (cairo_check_event(surface, 0))
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
                break;
        }

        nanosleep(&req, &req);
    }

    cairo_destroy(context);
    destroy(surface);

    return 0;
}
