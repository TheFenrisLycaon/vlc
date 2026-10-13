# Building heimdall
#
if BUILD_HEIMDALL
if HAVE_OSX
bin_PROGRAMS = heimdall-osx
noinst_PROGRAMS = heimdall-osx-static
else
bin_PROGRAMS = heimdall
noinst_PROGRAMS = heimdall-static
endif
endif
EXTRA_DIST = heimdall_win32_rc.rc.in
CLEANFILES = heimdall_win32_rc.rc

SUFFIXES = .rc.in .rc

AM_CPPFLAGS = $(CPPFLAGS_heimdall)
AM_CFLAGS = $(CFLAGS_heimdall)
noinst_LTLIBRARIES =
if HAVE_DBUS
AM_CPPFLAGS += -DHAVE_DBUS
endif
if HAVE_EMSCRIPTEN
AM_CFLAGS += -sMAIN_MODULE=1
endif

#
# Main Heimdall executable
#
heimdall_LDADD = ../lib/libheimdall.la ../src/libheimdallcore.la
heimdall_CPPFLAGS = $(AM_CPPFLAGS)
if !HAVE_WIN32
heimdall_SOURCES = heimdall.c override.c
heimdall_LDADD += $(LIBDL)
else
heimdall_SOURCES = winheimdall.c
EXTRA_heimdall_DEPENDENCIES = heimdall_win32_rc.$(OBJEXT)
heimdall_LDFLAGS = -mwindows -Wc,-static
heimdall_LDADD += -lole32 heimdall_win32_rc.$(OBJEXT)
libbreakpad_wrapper_la_SOURCES = breakpad.cpp
libbreakpad_wrapper_la_LIBADD = $(BREAKPAD_LIBS)
libbreakpad_wrapper_la_LDFLAGS = -static
libbreakpad_wrapper_la_CXXFLAGS = $(AM_CXXFLAGS) $(BREAKPAD_CFLAGS) -DBREAKPAD_URL=\"@BREAKPAD_URL@\"
if HAVE_BREAKPAD
noinst_LTLIBRARIES += libbreakpad_wrapper.la
heimdall_LDADD += libbreakpad_wrapper.la -lstdc++ -lwininet
EXTRA_heimdall_DEPENDENCIES += libbreakpad_wrapper.la
heimdall_CPPFLAGS += -DHAVE_BREAKPAD
endif
endif

heimdall_osx_SOURCES = darwinheimdall.m
heimdall_osx_CPPFLAGS = $(AM_CPPFLAGS)
heimdall_osx_LDFLAGS = $(LDFLAGS_heimdall) -Wl,-framework,CoreFoundation,-framework,Cocoa
heimdall_osx_LDFLAGS += -Xlinker -rpath -Xlinker "$(libdir)"
if HAVE_BREAKPAD
heimdall_osx_LDFLAGS += -Wl,-framework,Breakpad
heimdall_osx_OBJCFLAGS = -F$(CONTRIB_DIR)/Frameworks
heimdall_osx_CPPFLAGS += -DHAVE_BREAKPAD
endif
heimdall_osx_LDADD = ../lib/libheimdall.la

heimdall_osx_static_SOURCES = $(heimdall_osx_SOURCES)
heimdall_osx_static_CPPFLAGS = $(heimdall_osx_CPPFLAGS)
heimdall_osx_static_OBJCFLAGS = $(AM_OBJCFLAGS) \
	-DTOP_BUILDDIR=\"$(abs_top_builddir)\" \
	-DTOP_SRCDIR=\"$(abs_top_srcdir)\" \
	$(NULL)
heimdall_osx_static_LDFLAGS = $(heimdall_osx_LDFLAGS) -static
heimdall_osx_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../lib/.libs/"
heimdall_osx_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../src/.libs/"
heimdall_osx_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../Frameworks/"
heimdall_osx_static_LDADD = $(heimdall_osx_LDADD)
if HAVE_BREAKPAD
heimdall_osx_static_LDFLAGS += -Wl,-framework,Breakpad
heimdall_osx_static_OBJCFLAGS += -F$(CONTRIB_DIR)/Frameworks
endif


#
# Static (debug) Heimdall executable
#
heimdall_static_SOURCES = $(heimdall_SOURCES)
heimdall_static_CFLAGS = $(AM_CFLAGS) \
	-DTOP_BUILDDIR=\"$(abs_top_builddir)\" \
	-DTOP_SRCDIR=\"$(abs_top_srcdir)\" \
	$(NULL)
heimdall_static_CPPFLAGS = $(heimdall_CPPFLAGS)
EXTRA_heimdall_static_DEPENDENCIES = $(EXTRA_heimdall_DEPENDENCIES)
heimdall_static_LDADD = $(heimdall_LDADD)
heimdall_static_LDFLAGS = $(heimdall_LDFLAGS) -no-install -static

#
# Root wrapper
#
if BUILD_HEIMDALL
if !HAVE_WIN32
if !HAVE_OS2
bin_PROGRAMS += heimdall-wrapper
endif
endif
endif
heimdall_wrapper_SOURCES = rootwrap.c
heimdall_wrapper_LDADD = $(SOCKET_LIBS)

heimdall_win32_rc.rc: $(top_builddir)/config.status heimdall_win32_rc.rc.in
	cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="bin/$@"

heimdall_win32_rc.$(OBJEXT): heimdall_win32_rc.rc $(top_srcdir)/extras/package/win32/heimdall.exe.manifest
	$(WINDRES) --include-dir $(top_srcdir)/share/icons --include-dir $(top_srcdir)/extras/package/win32 -i $< -o $@

#
# Plug-ins cache generator
#
if HAVE_DYNAMIC_PLUGINS
pkglibexec_PROGRAMS = heimdall-cache-gen
endif
heimdall_cache_gen_SOURCES = cachegen.c
heimdall_cache_gen_LDADD = \
	$(GNUGETOPT_LIBS) \
	../compat/libcompat.la \
	../lib/libheimdall.la
if HAVE_WIN32
heimdall_cache_gen_LDADD += heimdall_win32_rc.$(OBJEXT)
heimdall_cache_gen_LDFLAGS = -Wc,-static
EXTRA_heimdall_cache_gen_DEPENDENCIES = heimdall_win32_rc.$(OBJEXT)
endif

if HAVE_PARTIAL_LINKING
AM_CPPFLAGS += -DHEIMDALL_PARTIAL_LINKING -I $(builddir)/../modules/
if !HAVE_WIN32
heimdall_SOURCES += heimdall_static_modules.c
endif
heimdall_osx_SOURCES += heimdall_static_modules.c
heimdall_LDADD += ../modules/libheimdall_static_modules.la
heimdall_osx_LDADD += ../modules/libheimdall_static_modules.la
endif

#
# Plug-ins cache
#
if HAVE_DYNAMIC_PLUGINS
noinst_DATA = ../modules/plugins.dat
endif
MOSTLYCLEANFILES = $(noinst_DATA)

if HAVE_OSX
if BUILD_HEIMDALL
install-data-local:
	cd "$(DESTDIR)$(bindir)"; mv heimdall-osx heimdall

endif
endif

.PHONY: ../modules/plugins.dat

../modules/plugins.dat: heimdall-cache-gen$(EXEEXT)
	$(AM_V_at)rm -f ../modules/plugins.dat
	$(AM_V_GEN)if test "$(build)" = "$(host)"; then \
		./heimdall-cache-gen$(EXEEXT) `cd "../modules" ; pwd -P` ; \
	else \
		echo "Cross-compilation: cache generation skipped!" ; \
	fi

include preparser/Makefile.am
