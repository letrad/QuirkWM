#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <toml.h>
#include "config.h"
#include "layout.h"
#include "types.h"

int strsplit(const char *txt, char delim, char ***tokens) {
    int *tklen, *t, count = 1;
    char **arr, *p = (char *) txt;

    while (*p != '\0') if (*p++ == delim) count += 1;
    t = tklen = calloc(count, sizeof (int));
    for (p = (char *) txt; *p != '\0'; p++) *p == delim ? *t++ : (*t)++;
    *tokens = arr = malloc(count * sizeof (char *));
    t = tklen;
    p = *arr++ = calloc(*(t++) + 1, sizeof (char *));
    while (*txt != '\0') {
        if (*txt == delim) {
            p = *arr++ = calloc (*(t++) + 1, sizeof (char *));
            txt++;
        }
        else *p++ = *txt++;
    }
    free(tklen);
    return count;
}

void spawn_program(const char *command) {
    if (fork() == 0) {
        // Child process
        if (fork() == 0) {
            // Grandchild process
            setsid(); // Create a new session

            // Make a copy of the command arguments
            size_t size_of_command = strlen(command);
            char *command_copy = malloc(size_of_command + 9);
            memcpy(command_copy, command, size_of_command);

            // Tokenize the command to pass to execvp

            strcat(command_copy, " replace");
            char **args = NULL;
            int count = strsplit(command_copy, ' ', &args);

            args[count - 1] = NULL; // the last argument is replace which we are nulling out for execve

            if (execvp(args[0], args) == -1) { // If execvp fails, it returns -1
                perror("Failed to launch program");
                exit(1);
            }

            // free memory in case execvp fails
            for (int i = 0; i < count - 1; i++) {
                free(args[i]);
            }

            free(args);
            free(command_copy);
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

void cleanup(WindowManager *wm) {
    // Free allocated memory for the window list
    WindowNode *current = wm->window_list;
    while (current) {
        WindowNode *next = current->next;
        free(current);
        current = next;
    }
    wm->window_list = NULL;
    wm->num_windows = 0;

    // Close the stringed configs:
    free(wm->config.terminal);
    free(wm->config.terminal);
}

unsigned int get_mod_mask(const char *modkey) {
    // Allocate memory for the temp string.
    // Size is length of the input string plus 1 for the null terminator.
    char temp[strlen(modkey) + 1];

    // Convert to lowercase.
    for (int i = 0; modkey[i]; i++) {
        temp[i] = tolower((unsigned char)modkey[i]);
    }

    // Null-terminate the string.
    temp[strlen(modkey)] = '\0';

    if (strcmp(temp, "alt") == 0) return Mod1Mask;
    if (strcmp(temp, "super") == 0) return Mod4Mask;
    if (strcmp(temp, "ctrl") == 0) return ControlMask;
    if (strcmp(temp, "shift") == 0) return ShiftMask;
    return 0;
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
    unsigned int mod_mask = get_mod_mask(wm.config.modkey);
    /* grab Super + Q and Super + T keys */
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_q), mod_mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_t), mod_mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_Left), mod_mask, wm.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(wm.dpy, XKeysymToKeycode(wm.dpy, XK_Right), mod_mask, wm.root, True, GrabModeAsync, GrabModeAsync);

    /* event loop */
    while (1) {
        XNextEvent(wm.dpy, &ev);

        if (ev.xany.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);

            // Check if the modifier key is held down
            if (ev.xkey.state & mod_mask) {
                if (keysym == XK_q) {
                    break;
                } else if (keysym == XK_t) {
                    spawn_program(wm.config.terminal);
                } else if (keysym == XK_Left) {
                    focus_previous_window(&wm);
                } else if (keysym == XK_Right) {
                    focus_next_window(&wm);
                }
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
    cleanup(&wm);
    XCloseDisplay(wm.dpy);
    return 0;

}
