#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <toml.h>
#include "config.h"
#include "layout.h"
#include "types.h"

void spawn_program(const char *command) {
    if (fork() == 0) {
        // Child process
        if (fork() == 0) {
            // Grandchild process
            setsid(); // Create a new session

            // Tokenize the command to pass to execvp
            char *args[64];
            int i = 0;

            char *command_copy = strdup(command);
            char *token = strtok(command_copy, " ");
            while (token) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            if (execvp(args[0], args) == -1) { // If execvp fails, it returns -1
                perror("Failed to launch program");
                exit(1);
            }

            free(command_copy); // free memory in case execvp fails
        }

        exit(0); // Exit child process
    }

    // Parent process waits for the child to exit
    wait(NULL);
}


void add_window(WindowManager *wm, Window w) {
    WindowNode *new_node = malloc(sizeof(WindowNode));
    if (new_node == NULL) {
        // FIXME: Unlikely, handle condition when there isn't enough memory
        // Right now, it just doesn't create a new window, even when there's none
        return;
    }
    new_node->window = w;
    new_node->next = NULL;

    if (wm->window_list == NULL) {
        wm->window_list = new_node;
    } else {
        WindowNode *current = wm->window_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    wm->num_windows++;
}

void remove_window(WindowManager *wm, Window w) {
    WindowNode *previous = NULL;
    WindowNode *current = wm->window_list;

    while (current != NULL) {
        if (current->window == w) {
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                wm->window_list = current->next;
            }
            free(current);
            wm->num_windows--;
            return;
        }
        previous = current;
        current = current->next;
    }
}


void set_last_focused_window(WindowManager *wm, Window w) {
    wm->last_focused_window = w;
}

void focus_next_window(WindowManager *wm) {
    if (wm->window_list && wm->num_windows > 1) {
        WindowNode *current = wm->window_list;
        WindowNode *next = current->next ? current->next : wm->window_list;

        while (current->window != wm->last_focused_window && current->next) {
            current = current->next;
            next = current->next ? current->next : wm->window_list;
        }

        XSetInputFocus(wm->dpy, next->window, RevertToParent, CurrentTime);
        set_last_focused_window(wm, next->window);
    }
}

void focus_previous_window(WindowManager *wm) {
    if (wm->window_list && wm->num_windows > 1) {
        WindowNode *previous = NULL;
        WindowNode *current = wm->window_list;

        while (current->window != wm->last_focused_window) {
            previous = current;
            current = current->next;
        }

        if (previous == NULL) {
            current = wm->window_list;
            while (current->next) current = current->next;
            previous = current;
        }

        XSetInputFocus(wm->dpy, previous->window, RevertToParent, CurrentTime);
        set_last_focused_window(wm, previous->window);
    }
}

void cleanup_windows(WindowManager *wm) {
    // Free allocated memory for the window list
    WindowNode *current = wm->window_list;
    while (current) {
        WindowNode *next = current->next;
        free(current);
        current = next;
    }
    wm->window_list = NULL;
    wm->num_windows = 0;
}


int main(void) {
    XEvent ev;
    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    WindowManager wm = {
            .dpy = dpy,
            .root = DefaultRootWindow(dpy),
            .window_list = NULL,
            .num_windows = 0,
            .config = get_config(),
            .last_focused_window = None
    };


    /* select kind of events we are interested in */
    XSelectInput(wm.dpy, wm.root, SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask);

    /* grab Super + Q and Super + T keys */
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_q), Mod4Mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_t), Mod4Mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_Left), Mod4Mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_Right), Mod4Mask, wm.root, True, GrabModeAsync, GrabModeAsync);

    /* event loop */
    while (1) {
        XNextEvent(wm.dpy, &ev);

        if (ev.xany.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);

            // Super + Q pressed
            if (keysym == XK_q && (ev.xkey.state & Mod4Mask)) {
                break;
            }
                // Super + T pressed
            else if (keysym == XK_t && (ev.xkey.state & Mod4Mask)) {

                spawn_program(wm.config.terminal);
            }
                // Arrow keys pressed (with Mod4 key)
            else if (keysym == XK_Left && (ev.xkey.state & Mod4Mask)) {
                focus_previous_window(&wm);
            } else if (keysym == XK_Right && (ev.xkey.state & Mod4Mask)) {
                focus_next_window(&wm);
            }
        } else if (ev.xany.type == MapRequest) {
            XMapRequestEvent *e = &ev.xmaprequest;
            printf("MapRequest event received, window ID: %lu\n", e->window);
            add_window(&wm, e->window);
            XMapWindow(wm.dpy, e->window);
            set_last_focused_window(&wm, e->window);
            tile_windows(&wm);

        } else if (ev.xany.type == UnmapNotify) {
            // Handling window unmap
            XUnmapEvent *e = &ev.xunmap;
            if (e->window == wm.last_focused_window) {
                wm.last_focused_window = None;
            }
            remove_window(&wm, e->window);
            tile_windows(&wm);
        }

    }

    // Clean up and close connection
    cleanup_windows(&wm);
    XCloseDisplay(wm.dpy);
    // Free string configs
    free(wm.config.terminal);
    return 0;

}
