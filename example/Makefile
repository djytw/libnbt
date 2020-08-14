OBJDIR = ../build/
TARGETDIR = ../target/
objects = $(patsubst %.c,$(TARGETDIR)%,$(wildcard *.c))

LIBS = z
CFLAGS = -Wall -g 
CC = gcc

all : $(objects)

$(objects) : $(TARGETDIR)% : %.c
	@mkdir -p $(TARGETDIR)
	$(CC) $< ../nbt.c -o $@ $(CFLAGS) $(patsubst %,-l%,$(LIBS)) -I..