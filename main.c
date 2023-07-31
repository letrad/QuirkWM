#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct WindowNode {
    Window window;
    struct WindowNode *next;
} WindowNode;

Display *dpy;
Window root;
WindowNode *window_list = NULL;
int num_windows = 0;
int gap = 10;

// Variable to store the last focused window
Window last_focused_window = None;

void spawn_kitty() {
    if (fork() == 0) {
        execlp("kitty", "kitty", NULL);
        fprintf(stderr, "Failed to launch kitty\n");
        exit(1);
    }
}

void add_window(Window w) {
    WindowNode *new_node = malloc(sizeof(WindowNode));
    new_node->window = w;
    new_node->next = window_list;
    window_list = new_node;
    num_windows++;
}

void remove_window(Window w) {
    WindowNode **current = &window_list;
    while (*current != NULL) {
        if ((*current)->window == w) {
            WindowNode *tmp = *current;
            *current = (*current)->next;
            free(tmp);
            num_windows--;
            return;
        }
        current = &((*current)->next);
    }
}

void tile_windows() {
    WindowNode *node = window_list;
    int i = 0;
    int screen_width = DisplayWidth(dpy, 0);
    int screen_height = DisplayHeight(dpy, 0);
    int window_width = screen_width / num_windows;
    while (node != NULL) {
        int x = i * (window_width + gap);
        int y = 0;
        int width = window_width - gap;
        int height = screen_height - 2 * gap;
        XMoveResizeWindow(dpy, node->window, x, y + gap, width, height);
        node = node->next;
        i++;
    }
}

void set_last_focused_window(Window w) {
    last_focused_window = w;
}

void focus_next_window() {
    if (window_list && num_windows > 1) {
        WindowNode *current = window_list;
        WindowNode *next = current->next ? current->next : window_list;

        while (current->window != last_focused_window && current->next) {
            current = current->next;
            next = current->next ? current->next : window_list;
        }

        XSetInputFocus(dpy, next->window, RevertToParent, CurrentTime);
        set_last_focused_window(next->window);
    }
}

void focus_previous_window() {
    if (window_list && num_windows > 1) {
        WindowNode *previous = NULL;
        WindowNode *current = window_list;

        while (current->window != last_focused_window) {
            previous = current;
            current = current->next;
        }

        if (previous == NULL) {
            current = window_list;
            while (current->next) current = current->next;
            previous = current;
        }

        XSetInputFocus(dpy, previous->window, RevertToParent, CurrentTime);
        set_last_focused_window(previous->window);
    }
}

int main(void) {
    XEvent ev;

    /* open connection with the server */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(dpy);

    /* select kind of events we are interested in */
    XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask);

    /* grab Super + Q and Super + T keys */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_t), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Left), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Right), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);

    /* event loop */
    while (1) {
        XNextEvent(dpy, &ev);

        if (ev.xany.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);

            // Super + Q pressed
            if (keysym == XK_q && (ev.xkey.state & Mod4Mask)) {
                break;
            }
                // Super + T pressed
            else if (keysym == XK_t && (ev.xkey.state & Mod4Mask)) {
                spawn_kitty();
            }
                // Arrow keys pressed (with Mod4 key)
            else if (keysym == XK_Left && (ev.xkey.state & Mod4Mask)) {
                focus_previous_window();
            } else if (keysym == XK_Right && (ev.xkey.state & Mod4Mask)) {
                focus_next_window();
            }
        } else if (ev.xany.type == MapRequest) {
            XMapRequestEvent *e = &ev.xmaprequest;
            add_window(e->window);
            XMapWindow(dpy, e->window);
            set_last_focused_window(e->window);
            tile_windows();
        } else if (ev.xany.type == UnmapNotify) {
            // Handling window unmap
            XUnmapEvent *e = &ev.xunmap;
            remove_window(e->window);
            tile_windows();
        }
    }

    // close connection
    XCloseDisplay(dpy);

    return 0;
}
