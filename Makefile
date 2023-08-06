# Get all .c files from src/ directory
SRC_DIR = ./src
SOURCES = $(wildcard $(SRC_DIR)/*.c) ./include/tomlc99/toml.c

OBJECTS = $(SOURCES:.c=.o)

LDFLAGS = -lX11

LOCINCDIRS = -I./include -I./include/tomlc99

OS := $(shell uname)

ifeq ($(OS), OpenBSD)
CC = cc
INCDIRS = -I/usr/X11R6/include -I/usr/local/include $(LOCINCDIRS)
LIBDIRS = -L/usr/X11R6/lib -L/usr/local/lib
else ifeq ($(OS), FreeBSD)
CC = cc
INCDIRS = -I/usr/local/include
LIBDIRS = -L/usr/local/lib
else ifeq ($(OS), NetBSD)
CC = cc
INCDIRS = -I/usr/X11R7/include -I/usr/pkg/include
LIBDIRS = -L/usr/X11R7/lib -L/usr/pkg/lib
LDFLAGS = $(LDFLAGS) -Wl,-R/usr/X11R7/lib,-R/usr/pkg/lib
else ifeq ($(OS), Linux)
CC = gcc
INCDIRS = -I/usr/include/X11 $(LOCINCDIRS)
LIBDIRS = -L/usr/lib/X11 -L/usr/local/lib
else
CC = cc
INCDIRS = -I/usr/local/include -I/usr/X11R6/include
LIBDIRS = -L/usr/local/lib -L/usr/X11R6/lib
endif

CFLAGS = -std=c11 -D_GNU_SOURCE $(INCDIRS) $(LIBDIRS)

OUTPUT = quirkwm

# Targets
all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(OUTPUT) $(LDFLAGS)
	rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUT) $(OBJECTS)
