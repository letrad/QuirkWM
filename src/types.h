#ifndef TYPES_H
#define TYPES_H
#include <X11/Xlib.h>
#include "config.h"

typedef struct WindowNode {
    Window window;
    struct WindowNode *next;
} WindowNode;

typedef struct {
    Display *dpy;
    Window root;
    WindowNode *window_list;
    int num_windows;
    wm_config config;
    Window last_focused_window;
} WindowManager;

#endif // TYPES_H
