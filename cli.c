// EXAMPLE IMPLEMENTATION OF A CLI
// this is meant to be like bspwm's bspc
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/quirkwm.fifo"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open FIFO");
        return 1;
    }

    write(fd, argv[1], strlen(argv[1]));
    close(fd);

    return 0;
}
