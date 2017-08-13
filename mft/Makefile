#
# versioning information
#
PKG_NAME = "mft"
VERSION = 0
PATCHLEVEL = 9.2
BUILD_DATE = $(shell date +%D)
AUTHOR = "newt@scyld.com"

#
# installation directories
#
BINDIR = "/usr/local/bin"
LIBDIR = "/usr/local/lib"
MANDIR = "/usr/local/man"

CFLAGS = -Wall -g -m32
CPPFLAGS = -I. -Iinclude

MFT_STATIC_LIBRARIES = libmft.a
MFT_LIBRARIES = $(MFT_STATIC_LIBRARIES) $(MFT_SHARED_LIBRARIES)

all: mft_config.h $(MFT_LIBRARIES)

mft_config.h: Makefile
	echo "#define MFT_VERSION \"${VERSION}.${PATCHLEVEL}\"" > $@
	echo "#define MFT_BUILD_DATE \"${BUILD_DATE}\"" >> $@
	echo "#define MFT_AUTHOR \"${AUTHOR}\"" >> $@

install: all

libmft.a: option.o log.o helper.o
	$(LD) -m elf_i386 -r --whole-archive -o $@ $^

clean:
	rm -f core `find . -name '*.[oas]' -print`
	rm -f mft_config.h
	rm -f $(MFT_BINARIES)
	rm -f $(MFT_LIBRARIES)
