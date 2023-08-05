# Get all .c files from src/ directory
SRC_DIR = ./src
SOURCES = $(wildcard $(SRC_DIR)/*.c) ./include/tomlc99/toml.c


OBJECTS = $(SOURCES:.c=.o)

CFLAGS = -std=c11 -D_GNU_SOURCE -I./include -I./include/tomlc99 -I/usr/include/X11
LDFLAGS = -lX11
OUTPUT = quirkwm

# Targets
all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	gcc $(CFLAGS) $(OBJECTS) -o $(OUTPUT) $(LDFLAGS)
	rm -f $(OBJECTS)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUT) $(OBJECTS)
