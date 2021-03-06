#ifndef X_H_INCLUDED
#define X_H_INCLUDED

// Map window and return surface for that window.
cairo_surface_t *
cairo_create_x11_surface(int x, int y, int w, int h);

int
check_x_event(cairo_surface_t *sfc, int block);

void x_resize_window(Display *dsp, Window win, int x, int y);

void destroy(cairo_surface_t *sfc);

#endif
