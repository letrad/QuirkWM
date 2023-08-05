#ifndef LAYOUT_H
#define LAYOUT_H
#include "types.h"
#include <X11/Xlib.h>

void move_resize_window(Display *dpy, Window w, int x, int y, int width, int height);

void calculate_window_dimensions(WindowManager *wm, int *master_width, int *stack_width, int screen_width);
void arrange_master_window(WindowManager *wm, WindowNode **node, int master_width, int screen_height);
void arrange_stack_windows(WindowManager *wm, WindowNode *node, int stack_width, int master_width, int screen_height);
void tile_windows(WindowManager *wm);

#endif // LAYOUT_H
