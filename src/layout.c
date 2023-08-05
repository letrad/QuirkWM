#include "layout.h"
#include "types.h"
#include <X11/Xlib.h>
void move_resize_window(Display *dpy, Window w, int x, int y, int width, int height) {
    XMoveResizeWindow(dpy, w, x, y, width, height);
}

void calculate_window_dimensions(WindowManager *wm, int *master_width, int *stack_width, int screen_width) {
    *master_width = (screen_width - wm->config.gap) / 2;
    *stack_width = (wm->num_windows > 1) ? (screen_width - *master_width - wm->config.gap * (wm->num_windows + 1)) /
                                           (wm->num_windows - 1) : 0;
}

void arrange_master_window(WindowManager *wm, WindowNode **node, int master_width, int screen_height) {
    if (*node != NULL) {
        move_resize_window(wm->dpy, (*node)->window, wm->config.gap, wm->config.gap, master_width - wm->config.gap,
                           screen_height - 2 * wm->config.gap);
        *node = (*node)->next;
    }
}

void arrange_stack_windows(WindowManager *wm, WindowNode *node, int stack_width, int master_width, int screen_height) {
    for (int i = 1; node != NULL; i++) {
        int x = master_width + wm->config.gap * i + stack_width * (i - 1);
        int y = wm->config.gap;
        int width = (node->next == NULL) ? DisplayWidth(wm->dpy, 0) - x - wm->config.gap : stack_width;
        int height = screen_height - 2 * wm->config.gap;

        move_resize_window(wm->dpy, node->window, x, y, width, height);
        node = node->next;
    }
}

void tile_windows(WindowManager *wm) {
    if (wm->num_windows == 0) return;

    int screen_width = DisplayWidth(wm->dpy, 0);
    int screen_height = DisplayHeight(wm->dpy, 0);
    int master_width, stack_width;

    calculate_window_dimensions(wm, &master_width, &stack_width, screen_width);

    if (wm->num_windows == 1) {
        move_resize_window(wm->dpy, wm->window_list->window, wm->config.gap, wm->config.gap, screen_width - 2 * wm->config.gap,
                           screen_height - 2 * wm->config.gap);
    } else {
        WindowNode *node = wm->window_list;
        arrange_master_window(wm, &node, master_width, screen_height);
        arrange_stack_windows(wm, node, stack_width, master_width, screen_height);
    }

    if (wm->last_focused_window != None) {
        XSetInputFocus(wm->dpy, wm->last_focused_window, RevertToParent, CurrentTime);
    }
}
