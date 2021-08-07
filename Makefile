.POSIX:
CFLAGS = `pkg-config --cflags json-c` -Wall -O2
LDFLAGS = `pkg-config --libs json-c`
all: swayrandr
clean: ; rm -rf swayrandr
swayrandr: swayrandr.c
