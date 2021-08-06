.POSIX:
CFLAGS = -Wall -O2
all: swayrandr
clean: ; rm -rf swayrandr
swayrandr: swayrandr.c
