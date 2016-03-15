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
    da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp),
            100,100,x,y,0,0,0);
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

int
main (int argc, char *argv[])
{
    char *font = NULL;
    char *string;
    bool  italic = false;
    int   width = 30;
    int   length = 300;
    int   margin = -1;
    int   upper = -1;

    int opt;
    while ((opt = getopt(argc, argv, "hs:f:m:u:w:l:i")) != -1) {
        switch(opt)
        {
            case 'h': help(); break;
            case 's': string = optarg; break;
            case 'f': font = optarg;  break;
            case 'm': margin = strtol(optarg, NULL, 10); break;
            case 'u': upper = strtol(optarg, NULL, 10);  break;
            case 'l': length = strtol(optarg, NULL, 10); break;
            case 'w': width = strtol(optarg, NULL, 10); break;
            case 'i': italic = true; break;
            default: help();
        }
    }

    // Option checking.
    if (!font) printf("Font is required\n");
    if (margin < 0) margin = 5;
    if (upper < 0) upper = 5;
    printf("italic: %d\n", italic);

    cairo_surface_t *surface;
    cairo_t *context;
    cairo_text_extents_t text;

    surface = cairo_create_x11_surface0(length, width);
    context = cairo_create(surface);

    int running;
    for (running = 1; running == 1;)
    {
        cairo_set_source_rgb(context, 1, 1, 1);
        cairo_paint(context);


        cairo_set_source_rgb(context, 0, 0, 0);
        cairo_select_font_face(context, font,
                (italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(context, 11);
        cairo_text_extents(context, string, &text);


        cairo_move_to(context, margin, text.height + upper);
        cairo_show_text(context, string);
        cairo_surface_flush(surface);


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
        sleep(1);
    }

    cairo_destroy(context);
    destroy(surface);

    return 0;
}
