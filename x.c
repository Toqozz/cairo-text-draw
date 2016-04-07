#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <stdbool.h>

#include "x.h"

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

void
destroy(cairo_surface_t *sfc)
{
    Display *dsp = cairo_xlib_surface_get_display(sfc);

    cairo_surface_destroy(sfc);
    XCloseDisplay(dsp);
}
