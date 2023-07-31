#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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

void spawn_program(const char *program) {
    if (fork() == 0) {
        // Child process
        if (fork() == 0) {
            // Grandchild process
            setsid(); // Create a new session
            execlp(program, program, NULL);
            perror("Failed to launch program");
            exit(1);
        }
        exit(0); // Exit child process
    }

    // Parent process waits for the child to exit
    wait(NULL);
}


void add_window(Window w) {
    WindowNode *new_node = malloc(sizeof(WindowNode));
    new_node->window = w;
    new_node->next = NULL;

    if (window_list == NULL) {
        window_list = new_node;
    } else {
        WindowNode *current = window_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
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
    if (num_windows == 0) return;

    // If only one window, make it occupy the entire screen, with a gap around it
    if (num_windows == 1) {
        XMoveResizeWindow(dpy, window_list->window, gap, gap, DisplayWidth(dpy, 0) - 2 * gap, DisplayHeight(dpy, 0) - 2 * gap);
        return;
    }

    WindowNode *node = window_list;
    int screen_width = DisplayWidth(dpy, 0);
    int screen_height = DisplayHeight(dpy, 0);
    int master_width = (screen_width - gap) / 2; // 50% for the master window
    int stack_width = (screen_width - master_width - gap * (num_windows - 1)) / (num_windows - 1);

    // Master window
    if (node != NULL) {
        XMoveResizeWindow(dpy, node->window, gap, gap, master_width - gap, screen_height - 2 * gap);
        node = node->next;
    }

    // Stack windows
    for (int i = 1; node != NULL; i++) {
        int x = master_width + gap * i + stack_width * (i - 1);
        int y = gap;
        int width = stack_width;
        int height = screen_height - 2 * gap;
        XMoveResizeWindow(dpy, node->window, x, y, width, height);
        node = node->next;
    }
    // Set focus to the last focused window
    if (last_focused_window != None) {
        XSetInputFocus(dpy, last_focused_window, RevertToParent, CurrentTime);
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
                spawn_program("kitty");
            }
                // Arrow keys pressed (with Mod4 key)
            else if (keysym == XK_Left && (ev.xkey.state & Mod4Mask)) {
                focus_previous_window();
            } else if (keysym == XK_Right && (ev.xkey.state & Mod4Mask)) {
                focus_next_window();
            }
        }
        else if (ev.xany.type == MapRequest) {
            XMapRequestEvent *e = &ev.xmaprequest;
            printf("MapRequest event received, window ID: %lu\n", e->window);
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
