OBJDIR = build/
TARGETDIR = target/

ZLIB ?= ZLIB
export ZLIB

.PHONY : clean 

all : example

clean :
	rm -rf $(OBJDIR)
	rm -rf $(TARGETDIR)

example : nbt.c nbt.h
	@cd example; make