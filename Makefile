#!/bin/sh
#
#  cpuinfo Makefile (C) 2005-2007 Gwenole Beauchesne
#
-include config.mak

ifeq ($(SRC_PATH),)
SRC_PATH = .
endif

PACKAGE = cpuinfo
ifeq ($(VERSION),)
VERSION := $(shell sed < $(SRC_PATH)/$(PACKAGE).spec -n '/^\%define version[	]*/s///p')
endif
ifeq ($(RELEASE),)
RELEASE := $(shell sed < $(SRC_PATH)/$(PACKAGE).spec -n '/^\%define release[	]*/s///p')
endif
ifeq ($(SVNDATE),)
SVNDATE := $(shell sed < $(SRC_PATH)/$(PACKAGE).spec -n '/^\%define svndate[ 	]*/s///p')
endif
ifeq ($(SVNDATE),)
SVNDATE := $(shell date '+%Y%m%d')
endif
ifeq ($(SNAPSHOT),)
SNAPSHOT := $(shell echo "$(RELEASE)" | grep "^0")
ifeq ($(SNAPSHOT),$(RELEASE))
SNAPSHOT := 1
endif
endif
ifeq ($(SNAPSHOT),1)
VERSION_SUFFIX = -$(SVNDATE)
endif

ifeq ($(LN),)
LN = ln
endif
ifeq ($(RANLIB),)
RANLIB = ranlib
endif
ifeq ($(INSTALL),)
INSTALL = install
endif
ifeq ($(PERL),)
PERL = perl
endif

CPPFLAGS	= -I. -I$(SRC_PATH)
PIC_CFLAGS	= -fPIC
DSO_LDFLAGS	= -shared
CC_FOR_SHARED	= $(CC)
ifeq ($(OS),darwin)
PIC_CFLAGS	= -fno-common
CC_FOR_SHARED	= DYLD_LIBRARY_PATH=. $(CC)
endif
ifeq ($(COMPILER),xlc)
PIC_CFLAGS	= -qpic
DSO_LDFLAGS	= -qmkshrobj
endif

libcpuinfo_a_needs_PIC	= no
ifeq ($(build_shared),no)
ifeq ($(build_perl),yes)
libcpuinfo_a_needs_PIC	= yes
endif
endif

libcpuinfo_a		= libcpuinfo.a
libcpuinfo_a_SOURCES	= debug.c cpuinfo-common.c cpuinfo-$(CPUINFO_ARCH).c
libcpuinfo_a_OBJECTS	= $(libcpuinfo_a_SOURCES:%.c=%.o)

libcpuinfo_so_major	= 1
libcpuinfo_so_minor	= 0
libcpuinfo_so		= libcpuinfo.so
libcpuinfo_so_SONAME	= $(libcpuinfo_so).$(libcpuinfo_so_major)
libcpuinfo_so_LTLIBRARY	= $(libcpuinfo_so).$(libcpuinfo_so_major).$(libcpuinfo_so_minor).0
libcpuinfo_so_LDFLAGS	= $(DSO_LDFLAGS) -Wl,-soname,$(libcpuinfo_so_SONAME)
ifeq ($(OS),darwin)
libcpuinfo_so		= libcpuinfo.dylib
libcpuinfo_so_SONAME	= libcpuinfo.$(libcpuinfo_so_major).dylib
libcpuinfo_so_VERSION	= $(libcpuinfo_so_major).$(libcpuinfo_so_minor).0
libcpuinfo_so_LTLIBRARY	= libcpuinfo.$(libcpuinfo_so_VERSION).dylib
libcpuinfo_so_LDFLAGS	= -dynamiclib \
			  -install_name $(libdir)/$(libcpuinfo_so_SONAME) \
			  -compatibility_version $(libcpuinfo_so_major).$(libcpuinfo_so_minor) \
			  -current_version $(libcpuinfo_so_VERSION)
endif
libcpuinfo_so_OBJECTS	= $(libcpuinfo_a_SOURCES:%.c=%.os)

libcpuinfo_module	= $(libcpuinfo_so)
ifeq ($(libcpuinfo_a_needs_PIC),yes)
libcpuinfo_a_OBJECTS	= $(libcpuinfo_so_OBJECTS)
libcpuinfo_module	= $(libcpuinfo_a)
endif

cpuinfo_PROGRAM	= cpuinfo
cpuinfo_SOURCES	= cpuinfo.c
cpuinfo_OBJECTS	= $(cpuinfo_SOURCES:%.c=%.o)
ifeq ($(build_shared),yes)
cpuinfo_DEPS	= $(libcpuinfo_so)
cpuinfo_LDFLAGS	= -L. -lcpuinfo
else
ifeq ($(build_static),yes)
cpuinfo_DEPS	= $(libcpuinfo_a)
cpuinfo_LDFLAGS	= $(libcpuinfo_a)
else
cpuinfo_OBJECTS += $(libcpuinfo_a_OBJECTS)
endif
endif

perl_bindings_DIR	= $(SRC_PATH)/src/bindings/perl
perl_bindings_LIB	= $(perl_bindings_DIR)/blib/arch/auto/Cpuinfo/Cpuinfo.so
perl_bindings_FILES	= $(patsubst %,$(perl_bindings_DIR)/%,$(shell cat $(perl_bindings_DIR)/MANIFEST))

TARGETS		= $(cpuinfo_PROGRAM)
ifeq ($(build_static),yes)
TARGETS		+= $(libcpuinfo_a)
endif
ifeq ($(build_shared),yes)
TARGETS		+= $(libcpuinfo_so)
endif
ifeq ($(build_perl),yes)
TARGETS		+= perl
endif

archivedir	= files/
SRCARCHIVE	= $(PACKAGE)-$(VERSION)$(VERSION_SUFFIX).tar
FILES		= configure Makefile $(PACKAGE).spec
FILES		+= README NEWS TODO COPYING COPYING.LIB ChangeLog
FILES		+= $(wildcard src/*.c)
FILES		+= $(wildcard src/*.h)
FILES		+= $(perl_bindings_FILES)

all: $(TARGETS)

clean: perl.clean
	rm -f $(TARGETS) *.o *.os
	rm -f $(libcpuinfo_a) $(libcpuinfo_a_OBJECTS)
	rm -f $(libcpuinfo_so) $(libcpuinfo_so_SONAME) $(libcpuinfo_so_LTLIBRARY) $(libcpuinfo_so_OBJECTS)

$(cpuinfo_PROGRAM): $(cpuinfo_OBJECTS) $(cpuinfo_DEPS)
	$(CC_FOR_SHARED) -o $@ $(cpuinfo_OBJECTS) $(cpuinfo_LDFLAGS) $(LDFLAGS)

install: install.dirs install.bins install.libs install.perl
install.dirs:
	mkdir -p $(DESTDIR)$(bindir)
ifeq (yes,$(findstring yes,$(build_static) $(build_shared)))
	mkdir -p $(DESTDIR)$(libdir)
endif
ifeq ($(install_sdk),yes)
	mkdir -p $(DESTDIR)$(includedir)
endif

install.bins: $(cpuinfo_PROGRAM)
	$(INSTALL) -m 755 $(INSTALL_STRIPPED) $(cpuinfo_PROGRAM) $(DESTDIR)$(bindir)/

install.libs: install.libs.static install.libs.shared install.headers
ifeq ($(build_static),yes)
install.libs.static: $(libcpuinfo_a)
	$(INSTALL) -m 644 $(INSTALL_STRIPPED) $< $(DESTDIR)$(libdir)/
else
install.libs.static:
endif
ifeq ($(build_shared),yes)
install.libs.shared: $(libcpuinfo_so)
	$(INSTALL) -m 755 $(INSTALL_STRIPPED) $(libcpuinfo_so_LTLIBRARY) $(DESTDIR)$(libdir)/
	$(LN) -sf $(libcpuinfo_so_LTLIBRARY) $(DESTDIR)$(libdir)/$(libcpuinfo_so_SONAME)
	$(LN) -sf $(libcpuinfo_so_SONAME) $(DESTDIR)$(libdir)/$(libcpuinfo_so)
else
install.libs.shared:
endif
ifeq ($(install_sdk),yes)
install.headers:
	$(INSTALL) -m 644 $(SRC_PATH)/src/cpuinfo.h $(DESTDIR)$(includedir)/
else
install.headers:
endif
ifeq ($(build_perl),yes)
install.perl: perl.install
else
install.perl:
endif

$(archivedir)::
	[ -d $(archivedir) ] || mkdir $(archivedir) > /dev/null 2>&1

tarball:
	$(MAKE) -C $(SRC_PATH) do_tarball
do_tarball: $(archivedir) $(archivedir)$(SRCARCHIVE).bz2

$(archivedir)$(SRCARCHIVE): $(archivedir) $(FILES)
	BUILDDIR=`mktemp -d /tmp/buildXXXXXXXX`						; \
	mkdir -p $$BUILDDIR/$(PACKAGE)-$(VERSION)					; \
	(cd $(SRC_PATH) && tar c $(FILES)) | tar x -C $$BUILDDIR/$(PACKAGE)-$(VERSION)	; \
	[ "$(SNAPSHOT)" = "1" ] && svndate_def="%" || svndate_def="#"			; \
	sed -e "s/^[%#]define svndate.*/$${svndate_def}define svndate $(SVNDATE)/" 	  \
	  < $(SRC_PATH)/$(PACKAGE).spec							  \
	  > $$BUILDDIR/$(PACKAGE)-$(VERSION)/$(PACKAGE).spec				; \
	(cd $$BUILDDIR && tar cvf $(SRCARCHIVE) $(PACKAGE)-$(VERSION))			; \
	mv -f $$BUILDDIR/$(SRCARCHIVE) $(archivedir)					; \
	rm -rf $$BUILDDIR
$(archivedir)$(SRCARCHIVE).bz2: $(archivedir)$(SRCARCHIVE)
	bzip2 -9vf $(archivedir)$(SRCARCHIVE)

RPMBUILD = \
	RPMDIR=`mktemp -d`								; \
	mkdir -p $$RPMDIR/{SPECS,SOURCES,BUILD,RPMS,SRPMS,tmp}				; \
	rpmbuild --define "_topdir $$RPMDIR" --define "_tmppath $$RPMDIR/tmp"		  \
		 -ta $(2) $(1) &&							  \
	find $$RPMDIR/ -name *.rpm -exec mv -f {} $(archivedir) \;			; \
	rm -rf $$RPMDIR

localrpm: $(archivedir)$(SRCARCHIVE).bz2
	$(call RPMBUILD,$<)

changelog: ../common/authors.xml
	svn_prefix=`svn info .|sed -n '/^URL *: .*\/svn\/\(.*\)$$/s//\1\//p'`; \
	svn2cl --strip-prefix=$$svn_prefix --authors=../common/authors.xml || :
changelog.commit:
	svn commit -m "Generated by svn2cl." ChangeLog

%.o: $(SRC_PATH)/src/%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

%.os: $(SRC_PATH)/src/%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS) $(PIC_CFLAGS)

$(libcpuinfo_a): $(libcpuinfo_a_OBJECTS)
	$(AR) rc $@ $(libcpuinfo_a_OBJECTS)
	$(RANLIB) $@

$(libcpuinfo_so): $(libcpuinfo_so_SONAME)
	$(LN) -sf $< $@
$(libcpuinfo_so_SONAME): $(libcpuinfo_so_LTLIBRARY)
	$(LN) -sf $< $@
$(libcpuinfo_so_LTLIBRARY): $(libcpuinfo_so_OBJECTS)
	$(CC) -o $@ $(libcpuinfo_so_OBJECTS) $(libcpuinfo_so_LDFLAGS)

perl: $(perl_bindings_LIB)
perl.clean:
	@[ -f $(perl_bindings_DIR)/Makefile ] && \
	$(MAKE) -C $(perl_bindings_DIR) realclean || :
perl.install: $(perl_bindings_LIB)
	@[ -f $(perl_bindings_DIR)/Makefile ] && \
	$(MAKE) -C $(perl_bindings_DIR) install DESTDIR=$(DESTDIR)
$(perl_bindings_LIB): $(libcpuinfo_module) $(perl_bindings_FILES)
$(perl_bindings_LIB): $(libcpuinfo_module) $(perl_bindings_FILES)
	@\
	cpuinfo_incdir="`cd $(SRC_PATH) && pwd`/src" ; \
	cpuinfo_libdir="$(PWD)" ; \
	cd $(perl_bindings_DIR) && \
	$(PERL) Makefile.PL \
		--cflags="$(CFLAGS)" \
		--cpuinfo-incdir="$${cpuinfo_incdir}" \
		--cpuinfo-libdir="$${cpuinfo_libdir}" && \
	$(MAKE)
