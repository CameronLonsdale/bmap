#
# versioning information
#
PKG_NAME = "bmap"
VERSION = 1
PATCHLEVEL = 0.18
BUILD_DATE = $(shell date +%D)
AUTHOR = "newt@scyld.com"

#
# installation directories
#
BINDIR = "/usr/local/bin"
MANDIR = "/usr/local/man"

#
# dependancy directories
#
MFT_SOURCE_DIR=mft
MFT_LIB_DIR=mft
MFT_INCLUDE_DIR=mft/include


BOGUS_MAJOR = 123
BOGUS_MINOR = 123
BOGUS_FILENAME = "/.../image"

CFLAGS = -Wall -g -m32
CPPFLAGS = -I$(MFT_INCLUDE_DIR) -Iinclude
LDFLAGS = -L$(MFT_LIB_DIR) -lmft -m32

BINARIES = bmap slacker
LIBRARIES = $(STATIC_LIBRARIES) $(SHARED_LIBRARIES)

all: binaries doc

binaries: config.h mft dev_builder $(BINARIES) $(LIBRARIES)

config.h: Makefile
	echo "#ifndef NEWT_CONFIG_H" > $@
	echo "#define NEWT_CONFIG_H" >> $@
	echo "#define VERSION \"${VERSION}.${PATCHLEVEL}\"" >> $@
	echo "#define BUILD_DATE \"${BUILD_DATE}\"" >> $@
	echo "#define AUTHOR \"${AUTHOR}\"" >> $@
	echo "#define BMAP_BOGUS_MAJOR ${BOGUS_MAJOR}" >> $@
	echo "#define BMAP_BOGUS_MINOR ${BOGUS_MINOR}" >> $@
	echo "#define BMAP_BOGUS_FILENAME \"${BOGUS_FILENAME}\"" >> $@
	echo "#define _FILE_OFFSET_BITS 64" >> $@	
	echo "#endif" >>$@

install: all
	for i in $(BINARIES) ; do install -m 755 $$i $(BINDIR)/$$i ; done
	for i in $(BINARIES) ; do ./$$i --man > $(MANDIR)/man1/$$i.1 ; done

dev_entries.c: dev_builder
	./dev_builder > dev_entries.c

mft: dummy
	if [ -n $(MFT_SOURCE_DIR) ] ; then $(MAKE) -C $(MFT_SOURCE_DIR) ; fi

bmap: bmap.o libbmap.o dev_entries.o

slacker: slacker.o slacker-modules.o libbmap.o dev_entries.o

clean:
	rm -f *.[oas]
	rm -f *.dvi
	rm -f config.h
	rm -f dev_builder
	rm -f dev_entries.c
	rm -f bmap.sgml
	for i in $(BINARIES) ; do rm -f $$i-invoke.sgml ; done
	rm -f $(BINARIES)
	if [ -n $(MFT_SOURCE_DIR) ] ; then $(MAKE) -C $(MFT_SOURCE_DIR) clean ; fi

doc: binaries
	for i in $(BINARIES) ; do ./$$i --sgml > $$i-invoke.sgml ; done
	m4 < bmap.sgml.m4 > bmap.sgml
	sgml2latex bmap.sgml

release: clean
	cvs commit -m "see README" && cvs rtag release-"${VERSION}-`echo ${PATCHLEVEL} | tr . - `" ${PKG_NAME}

bmap.lsm: clean
	echo "Begin3" >> ../bmap.lsm
	echo "
	echo "End"
dummy:
