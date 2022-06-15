OBJDIR = ../build/
TARGETDIR = ../target/
objects = $(patsubst %.c,$(TARGETDIR)%,$(wildcard *.c))

ifeq ($(ZLIB), LIBDEFLATE)
LIBS = 
STATIC_LIBS = ../libdeflate/libdeflate.a
CFLAGS = -Wall -g -DLIBNBT_USE_LIBDEFLATE 
all : $(objects)
else
LIBS = z
STATIC_LIBS = 
CFLAGS = -Wall -g 
LIBRARY = .
all : $(objects)
endif

CC = gcc

$(objects) : $(TARGETDIR)% : %.c ../nbt.c
	@mkdir -p $(TARGETDIR)
	$(CC) $< ../nbt.c $(STATIC_LIBS) -o $@ $(CFLAGS) $(patsubst %,-l%,$(LIBS)) $(patsubst %,-L%,$(LIBRARY)) -I..
