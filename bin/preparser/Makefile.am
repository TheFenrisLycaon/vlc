AUTOMAKE_OPTIONS = subdir-objects

#
# Preparser executable
#
if BUILD_HEIMDALL
if HAVE_EXTERNAL_PREPARSER
if HAVE_DYNAMIC_PLUGINS
pkglibexec_PROGRAMS += heimdall-preparser
else
pkglibexec_PROGRAMS = heimdall-preparser
endif
noinst_PROGRAMS += heimdall-preparser-static
endif
endif

heimdall_preparser_SOURCES = \
		../src/config/getopt.c \
		../src/config/jaro_winkler.c \
		preparser/cmdline.c \
		preparser/cmdline.h \
		preparser/main.c

heimdall_preparser_CFLAGS = $(heimdall_CFLAGS)

heimdall_preparser_LDADD = \
		../compat/libcompat.la \
		$(SOCKET_LIBS) \
		$(heimdall_LDADD)

heimdall_preparser_LDFLAGS = $(heimdall_LDFLAGS)
heimdall_preparser_DEPENDENCIES = \
		$(heimdall_DEPENDENCIES)

if HAVE_OSX
heimdall_preparser_LDFLAGS += -Wl,-framework,CoreFoundation,-framework,Cocoa
heimdall_preparser_LDFLAGS += -Xlinker -rpath -Xlinker "$(libdir)"
if HAVE_BREAKPAD
heimdall_preparser_LDFLAGS += -Wl,-framework,Breakpad
heimdall_preparser_OBJCFLAGS = -F$(CONTRIB_DIR)/Frameworks
heimdall_preparser_CPPFLAGS = -DHAVE_BREAKPAD
endif
endif

if HAVE_WIN32
heimdall_preparser_LDFLAGS += -Wc,-static
endif


heimdall_preparser_static_SOURCES = $(heimdall_preparser_SOURCES)
heimdall_preparser_static_CFLAGS = $(AM_CFLAGS) \
	-DTOP_BUILDDIR=\"$(abs_top_builddir)\" \
	-DTOP_SRCDIR=\"$(abs_top_srcdir)\" \
	$(NULL)

heimdall_preparser_static_LDADD = $(heimdall_preparser_LDADD)
if HAVE_OSX
heimdall_preparser_static_LDFLAGS = $(heimdall_preparser_LDFLAGS) -static
heimdall_preparser_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../lib/.libs/"
heimdall_preparser_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../src/.libs/"
heimdall_preparser_static_LDFLAGS += -Xlinker -rpath -Xlinker "@executable_path/../Frameworks/"
else
heimdall_preparser_static_LDFLAGS = $(heimdall_preparser_LDFLAGS) -no-install -static
endif
