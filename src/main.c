#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <toml.h>
#include <setjmp.h>

typedef struct WindowNode {
    Window window;
    struct WindowNode *next;
} WindowNode;

typedef struct {
    int gap;
    char *terminal;
} wm_config;


typedef struct {
    Display *dpy;
    Window root;
    WindowNode *window_list;
    int num_windows;
    wm_config config;
    Window last_focused_window;
} WindowManager;

jmp_buf jump_buffer;

void handle_segfault(int sig) {
    longjmp(jump_buffer, 1);
}

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

            execvp(args[0], args);
            free(command_copy); // Free the duplicated string
            perror("Failed to launch program");
            exit(1);
        }
        exit(0); // Exit child process
    }

    // Parent process waits for the child to exit
    wait(NULL);
}


void add_window(WindowManager *wm, Window w) {
    WindowNode *new_node = malloc(sizeof(WindowNode));
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


void move_resize_window(Display *dpy, Window w, int x, int y, int width, int height) {
    XMoveResizeWindow(dpy, w, x, y, width, height);
}

void tile_windows(WindowManager *wm) {
    if (wm->num_windows == 0) return;

    int screen_width = DisplayWidth(wm->dpy, 0);
    int screen_height = DisplayHeight(wm->dpy, 0);

    // If only one window is open, use the full screen space
    if (wm->num_windows == 1) {
        WindowNode *node = wm->window_list;
        if (node != NULL) {
            move_resize_window(wm->dpy, node->window, wm->config.gap, wm->config.gap, screen_width - 2 * wm->config.gap,
                               screen_height - 2 * wm->config.gap);
        }
        return; // Early return since there is only one window
    }

    int master_width = (screen_width - wm->config.gap) / 2; // 50% for the master window
    int stack_width = (wm->num_windows > 1) ? (screen_width - master_width - wm->config.gap * (wm->num_windows + 1)) /
                                              (wm->num_windows - 1) : 0;

    // Master window
    WindowNode *node = wm->window_list;
    if (node != NULL) {
        move_resize_window(wm->dpy, node->window, wm->config.gap, wm->config.gap, master_width - wm->config.gap,
                           screen_height - 2 * wm->config.gap);
        node = node->next;
    }

    // Stack windows
    for (int i = 1; node != NULL; i++) {
        int x = master_width + wm->config.gap * i + stack_width * (i - 1);
        int y = wm->config.gap;
        int width = (node->next == NULL) ? screen_width - x - wm->config.gap : stack_width;
        int height = screen_height - 2 * wm->config.gap;

        move_resize_window(wm->dpy, node->window, x, y, width, height);
        node = node->next;
    }

    // Set focus to the last focused window
    if (wm->last_focused_window != None) {
        XSetInputFocus(wm->dpy, wm->last_focused_window, RevertToParent, CurrentTime);
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

wm_config default_config() {
    wm_config config;
    config.gap = 32;
    config.terminal = "kitty";
    return config;
}


wm_config get_config() {
    wm_config config;

    FILE *config_file;
    char errbuf[200];
    signal(SIGSEGV, handle_segfault);

    char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    char config_path[200];

    if (xdg_config_home) {
        snprintf(config_path, sizeof(config_path), "%s/quirk/config.toml", xdg_config_home);
    } else {
        snprintf(config_path, sizeof(config_path), "%s/.config/quirk/config.toml", getenv("HOME"));
    }

    config_file = fopen(config_path, "r");
    if (!config_file) {
        printf("There is no config file.");
        return default_config();
    }

    toml_table_t *conf = toml_parse_file(config_file, errbuf, sizeof(errbuf));
    if (!conf) {
        printf("Parsing error: %s\n", errbuf);
        return default_config();
    }

    toml_table_t *wm = toml_table_in(conf, "wm");
    if (wm && setjmp(jump_buffer) == 0) {
        toml_datum_t tmp = toml_int_in(wm, "gap");
        if (tmp.ok && tmp.u.i != 0) {
            config.gap = tmp.u.i;
        } else {
            config.gap = default_config().gap;
        }
    } else {
        config.gap = default_config().gap;
    }

    toml_table_t *preferences = toml_table_in(conf, "preferences");
    if (preferences && setjmp(jump_buffer) == 0) {
        toml_datum_t tmp = toml_string_in(preferences, "terminal");
        if (tmp.ok && tmp.u.s != NULL) {
            config.terminal = tmp.u.s;
        } else {
            config.terminal = default_config().terminal;
        }
    } else {
        config.terminal = default_config().terminal;
    }

    toml_free(conf);
    fclose(config_file);

    return config;
}


int main(void) {
    XEvent ev;
    wm_config wmConfig;
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

    return 0;
}
