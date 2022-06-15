OBJDIR = build/
TARGETDIR = target/

ZLIB ?= ZLIB
export ZLIB

.PHONY : all clean example libdeflate

ifeq ($(ZLIB), LIBDEFLATE)
all : libdeflate example
else
all : example
endif

clean :
	rm -rf $(OBJDIR)
	rm -rf $(TARGETDIR)

example :
	$(MAKE) -C example

libdeflate :
	$(MAKE) -C libdeflate
