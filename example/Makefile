OBJDIR = ../build/
TARGETDIR = ../target/
objects = $(patsubst %.c,$(TARGETDIR)%,$(wildcard *.c))

ifeq ($(ZLIB), LIBDEFLATE)
LIBS = deflate
CFLAGS = -Wall -g -DLIBNBT_USE_LIBDEFLATE 
LIBRARY = ../libdeflate/ .
all : $(objects) copylibrary
else
LIBS = z
CFLAGS = -Wall -g 
LIBRARY = .
all : $(objects)
endif

CC = gcc

$(objects) : $(TARGETDIR)% : %.c ../nbt.c
	@mkdir -p $(TARGETDIR)
	$(CC) $< ../nbt.c -o $@ $(CFLAGS) $(patsubst %,-l%,$(LIBS)) $(patsubst %,-L%,$(LIBRARY)) -I..

copylibrary :
	cp ../libdeflate/libdeflate.so* ../target/
