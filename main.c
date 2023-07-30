#include <X11/Xlib.h>
#include <X11/keysym.h> // I changed to just relying on something like sxkhd
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATH "/tmp/quirkwm.fifo"

typedef struct WindowNode {
    Window window;
    struct WindowNode *next;
} WindowNode;

Display *dpy;
Window root;
WindowNode *window_list = NULL;
int num_windows = 0;
int gap = 10;

// Globals here
int screen_width;
int screen_height;

Window last_focused_window = None;
WindowNode *current_focused_node = NULL;

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
    int screen_width = DisplayWidth(dpy, 0);
    int screen_height = DisplayHeight(dpy, 0);

    if (num_windows == 1) {
        // If there's only one window, it occupies the full screen.
        XMoveResizeWindow(dpy, node->window, gap, gap, screen_width - 2 * gap, screen_height - 2 * gap);
    } else if (num_windows > 1) {
        // Calculate the master and stack areas' dimensions based on the number of windows and the gap size.
        int master_width = (screen_width - (num_windows - 1) * gap) / 2;
        int stack_width = (screen_width - (num_windows - 1) * gap) / 2;

        // Resize and position the master window.
        XMoveResizeWindow(dpy, node->window, gap, gap, master_width - gap, screen_height - 2 * gap);
        node = node->next;

        // Tile the remaining windows vertically in the stack area.
        int stack_height = (screen_height - (num_windows - 1) * gap) / num_windows;
        int i = 0;
        while (node != NULL) {
            int x = master_width + 2 * gap; // Shift stack windows to the right of the master.
            int y = i * (stack_height + gap) + gap;
            int width = stack_width - gap;
            int height = stack_height;
            XMoveResizeWindow(dpy, node->window, x, y, width, height);
            node = node->next;
            i++;
        }
    }
}



void set_last_focused_window(Window w) {
    WindowNode *node = window_list;
    while (node != NULL) {
        if (node->window == w) {
            current_focused_node = node;
            break;
        }
        node = node->next;
    }
}

void focus_next_window() {
    if (current_focused_node && current_focused_node->next) {
        current_focused_node = current_focused_node->next;
    } else {
        current_focused_node = window_list; // Wrap around to the beginning
    }
    XSetInputFocus(dpy, current_focused_node->window, RevertToParent, CurrentTime);
}

void focus_previous_window() {
    if (current_focused_node == window_list || !current_focused_node) {
        current_focused_node = window_list;
        while (current_focused_node->next) current_focused_node = current_focused_node->next; // Go to the end
    } else {
        WindowNode *prev = window_list;
        while (prev->next != current_focused_node) prev = prev->next;
        current_focused_node = prev;
    }
    XSetInputFocus(dpy, current_focused_node->window, RevertToParent, CurrentTime);
}

void handle_command(const char *cmd) {
    // basic for now. works with FIFO
    if (strcmp(cmd, "spawn_kitty") == 0) {
        spawn_kitty();
    } else if (strcmp(cmd, "focus_next_window") == 0) {
        focus_next_window();
    } else if (strcmp(cmd, "focus_previous_window") == 0) {
        focus_previous_window();
    }
}

int main(void) {
    XEvent ev;
    int fifo_fd;

    // Create the FIFO
    mkfifo(FIFO_PATH, 0666);

    // Open the FIFO in non-blocking mode
    fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("Failed to open FIFO");
        exit(1);
    }

    /* open connection with the server */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(dpy);

    // Get the screen dimensions once and store them in global variables
    screen_width = DisplayWidth(dpy, 0);
    screen_height = DisplayHeight(dpy, 0);
    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(ConnectionNumber(dpy), &fds);
        FD_SET(fifo_fd, &fds);

        select(FD_SETSIZE, &fds, NULL, NULL, NULL);

        if (FD_ISSET(ConnectionNumber(dpy), &fds)) {
            // Handle X11 events
            XNextEvent(dpy, &ev);

            if (ev.xany.type == MapRequest) {
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

        if (FD_ISSET(fifo_fd, &fds)) {
            // Read and handle a command from the FIFO
            char cmd[256];
            int len = read(fifo_fd, cmd, sizeof(cmd) - 1);
            if (len > 0) {
                cmd[len] = '\0'; // Null-terminate the string
                handle_command(cmd);
            }
        }
    }

    close(fifo_fd);
    unlink(FIFO_PATH);
    XCloseDisplay(dpy);

    return 0;
}
