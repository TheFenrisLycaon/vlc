###############################################################################
# Automake targets and declarations
###############################################################################

# SUBDIRS stores the directories where a "make" is required when building
# something. DIST_SUBDIRS stores the directories where nothing is built but
# which have makefiles with distribution information.
#  - src (libheimdallcore) is nedeed by modules
SUBDIRS = compat po share src modules lib doc bin test
DIST_SUBDIRS = m4 $(SUBDIRS)

EXTRA_DIST = \
	buildsystem/cargo-test.py \
	buildsystem/check_qml_module.py \
	extras/include/x86/x86inc.asm \
	extras/include/x86/x86util.asm \
	extras/package/macosx/package.mak \
	extras/package/win32/package.mak \
	extras/package/npapi.am

dist_noinst_SCRIPTS = bootstrap
nodist_noinst_SCRIPTS = compile

BUILT_SOURCES_distclean =

BUILT_SOURCES = $(BUILT_SOURCES_distclean)

SUFFIXES =

AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-fast-install \
	--disable-alsa \
	--disable-avcodec --disable-avformat \
	--disable-postproc --disable-swscale \
	--disable-dbus \
	--disable-mad \
	--disable-faad --disable-skins2 \
	--disable-live555 \
	--disable-lua \
	--disable-fribidi \
	--with-kde-solid='$${datadir}/kde4/apps'

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = \
	1.11 \
	-Wall \
	check-news \
	dist-xz \
	no-dist-gzip
#	std-options

ChangeLog: Makefile.am
	rm -f -- "$@"
	cd doc && $(MAKE) $(AM_MAKEFLAGS) changelogs
	$(LN_S) -f doc/ChangeLog-2024 "$@"


###############################################################################
# tools (needed for contrib)
##############################################################################
EXTRA_DIST += \
	extras/tools/bootstrap \
	extras/tools/packages.mak \
	extras/tools/tools.mak \
	extras/tools/SHA512SUMS \
	extras/tools/automake-clang.patch \
	extras/tools/automake-disable-documentation.patch \
	extras/tools/gettext-no-iconv.patch \
	extras/tools/libtool-2.4.7-bitcode.patch \
	extras/tools/libtool-2.4.7-lpthread.patch \
	extras/tools/libtool-2.6.2-embed-bitcode.patch \
	extras/tools/libtool-2.5.4-ltmain.in-reference-MAGIC_EXE-in-the-main.patch \
	extras/tools/libtool-2.5.4-libtool.m4-respect-the-provided-LTCFLAGS-LTCC-during.patch \
	extras/tools/pkg-config-stdc23-port.patch

###############################################################################
# Various utilities ( editor syntax files, D-Bus controller ... )
##############################################################################
EXTRA_DIST += \
	extras/analyser/emacs.init \
	extras/analyser/heimdall.vim \
	extras/analyser/valgrind.suppressions \
	extras/buildsystem/make.pl \
	extras/misc/mpris.py \
	extras/misc/mpris.xml

###############################################################################
# Scripts for building dependencies.
##############################################################################
EXTRA_DIST += \
	contrib/bootstrap \
	contrib/src

###############################################################################
# Building libheimdall
###############################################################################

CLEANFILES =
DISTCLEANFILES = $(BUILT_SOURCES_distclean) compile doltcompile doltlibtool
MAINTAINERCLEANFILES = ChangeLog

# Shortcut for developers to rebuild the core (libheimdall + heimdall)
# Don't use it if you don't know what it is about.
# Don't complain if it doesn't work. -- Courmisch
libcompat:
	cd compat && $(MAKE) $(AM_MAKEFLAGS)

libheimdallcore: libcompat
	cd src && $(MAKE) $(AM_MAKEFLAGS) libheimdallcore.la

libheimdall: libheimdallcore
	cd lib && $(MAKE) $(AM_MAKEFLAGS) libheimdall.la

core: libheimdall heimdall$(EXEEXT)
	cd bin && $(MAKE) $(AM_MAKEFLAGS) heimdall$(EXEEXT) heimdall-static$(EXEEXT)

doc:
	cd doc && $(MAKE) $(AM_MAKEFLAGS) doc

.PHONY: libheimdall core doc

###############################################################################
# Building aliases
###############################################################################

ALL_ALIASES = cheimdall rheimdall sheimdall qheimdall nheimdall
if BUILD_HEIMDALL
bin_SCRIPTS = $(ALIASES)
endif
CLEANFILES += $(ALIASES) $(noinst_SCRIPTS)
EXTRA_SCRIPTS = $(ALL_ALIASES)

dist_noinst_SCRIPTS += make-alias

MKALIAS = bindir="$(bindir)" transform="$(transform)" program_prefix="$(program_prefix)" program_suffix="$(program_suffix)" $(top_srcdir)/make-alias $@

cheimdall: make-alias Makefile
	$(AM_V_GEN)$(MKALIAS) dummy

rheimdall: make-alias Makefile
	$(AM_V_GEN)$(MKALIAS) rc

sheimdall: make-alias Makefile
	$(AM_V_GEN)$(MKALIAS) skins2

qheimdall: make-alias Makefile
	$(AM_V_GEN)$(MKALIAS) qt

nheimdall: make-alias Makefile
	$(AM_V_GEN)$(MKALIAS) ncurses

if BUILD_HEIMDALL
noinst_SCRIPTS = heimdall$(EXEEXT)
endif

heimdall$(EXEEXT):
if HAVE_DARWIN
	$(AM_V_GEN)$(LN_S) -f bin/heimdall-osx-static heimdall
else
	$(AM_V_GEN)$(LN_S) -f bin/heimdall-static$(EXEEXT) heimdall$(EXEEXT)
endif

TESTS = test/run_heimdall.sh
dist_noinst_SCRIPTS += test/run_heimdall.sh

if BUILD_HEIMDALL
###############################################################################
# Installing plugins cache
###############################################################################
install-exec-hook:
	if test "$(build)" = "$(host)"; then \
		PATH="$(DESTDIR)$(bindir):$$PATH" \
		LD_LIBRARY_PATH="$(DESTDIR)$(libdir):$$LD_LIBRARY_PATH" \
		DYLD_LIBRARY_PATH="$(DESTDIR)$(libdir):$$DYLD_LIBRARY_PATH" \
		"$(DESTDIR)$(pkglibexecdir)/heimdall-cache-gen$(EXEEXT)" \
			 "$(DESTDIR)$(pkglibdir)/plugins" ; \
	else \
		echo "Cross-compilation: cache generation skipped!" ; \
	fi
endif

uninstall-hook:
	rm -f -- "$(DESTDIR)$(pkglibdir)/plugins/plugins.dat"

###############################################################################
# Test coverage
###############################################################################

lcov-raw.out:
	$(MAKE) $(AM_MAKEFLAGS) all
	lcov -z -d .
	$(MAKE) $(AM_MAKEFLAGS) check
	lcov -c -d . -o lcov-raw.out

lcov.out: lcov-raw.out
	lcov -r lcov-raw.out -o lcov.out \
		'*test*' 'contrib/*' '/usr/include/*' '*mock*'

lcov: lcov.out
	rm -Rf lcov lcov.tmp
	prefix="$$(cd "$(top_srcdir)" && pwd)" ; \
	genhtml -p "$$prefix" -o lcov.tmp lcov.out >/dev/null
	mv lcov.tmp lcov

.PHONY: lcov-raw.out


###############################################################################
# PO translation files update
###############################################################################
.PHONY: update-po

update-po:
	cd po && $(MAKE) POTFILES heimdall.pot update-po

###############################################################################
# OS Packaging rules
###############################################################################
include extras/package/macosx/package.mak
include extras/package/win32/package.mak
include extras/package/win32/msi.mak

dist-hook:
	chmod u+w "$(distdir)/configure" "$(distdir)/configure.ac"
	mtime_ref=$(mktemp)
#	We keep a copy to have a reference timestamp and restore it,
#	to avoid triggering automake again
	cp -p "$(distdir)/configure.ac" "$mtime_ref"
	sed -i '/^VERSION_DATE=\$$(/,/^)/c\VERSION_DATE='$(VERSION_DATE) \
		"$(distdir)/configure" "$(distdir)/configure.ac"
	touch -r "$mtime_ref" "$(distdir)/configure.ac"
	rm "$mtime_ref"
