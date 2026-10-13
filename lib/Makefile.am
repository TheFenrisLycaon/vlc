AUTOMAKE_OPTIONS = subdir-objects
pkgconfigdir = $(libdir)/pkgconfig
AM_CFLAGS = $(CFLAGS_libheimdall)

SUFFIXES = .pc.in .pc .rc.in .rc

BUILT_SOURCES = $(nodist_pkginclude_HEADERS)
CLEANFILES = $(BUILT_SOURCES) $(pkgconfig_DATA)

pkginclude_HEADERS = \
	../include/heimdall/libheimdall.h \
	../include/heimdall/libheimdall_dialog.h \
	../include/heimdall/libheimdall_downloader.h \
	../include/heimdall/libheimdall_media.h \
	../include/heimdall/libheimdall_media_discoverer.h \
	../include/heimdall/libheimdall_media_list.h \
	../include/heimdall/libheimdall_media_list_player.h \
	../include/heimdall/libheimdall_media_player.h \
	../include/heimdall/libheimdall_media_track.h \
	../include/heimdall/libheimdall_parser.h \
	../include/heimdall/libheimdall_renderer_discoverer.h \
	../include/heimdall/libheimdall_picture.h \
	../include/heimdall/libheimdall_version.h \
	../include/heimdall/libheimdall_video.h \
	../include/heimdall/heimdall.h

lib_LTLIBRARIES = libheimdall.la

libheimdall_la_SOURCES = \
	libheimdall_internal.h \
	media_internal.h \
	media_list_internal.h \
	media_player_internal.h \
	picture_internal.h \
	renderer_discoverer_internal.h \
	core.c \
	dialog.c \
	downloader.c \
	renderer_discoverer.c \
	error.c \
	log.c \
	video.c \
	audio.c \
	media.c \
	media_track.c \
	media_player.c \
	media_list.c \
	media_list_path.h \
	media_list_player.c \
	media_discoverer.c \
	parser.c \
	picture.c \
	libheimdall.pc.in \
	libheimdall.sym \
	../src/revision.c
EXTRA_DIST =

if HAVE_WIN32
noinst_DATA = libheimdall_win32_rc.rc
endif
EXTRA_DIST += libheimdall_win32_rc.rc.in

libheimdall_win32_rc.rc: libheimdall_win32_rc.rc.in $(top_builddir)/config.status
	$(AM_V_GEN)cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="lib/$@"

libheimdall_la_LIBADD = ../src/libheimdallcore.la ../compat/libcompat.la $(LIBM)
libheimdall_la_LDFLAGS = \
	$(LDFLAGS_libheimdall) \
	-no-undefined \
	-version-number @LIBHEIMDALL_ABI_MAJOR@:@LIBHEIMDALL_ABI_MINOR@:@LIBHEIMDALL_ABI_MICRO@ \
	-export-symbols $(srcdir)/libheimdall.sym
EXTRA_libheimdall_la_DEPENDENCIES = libheimdall.sym
libheimdall_la_CPPFLAGS = $(AM_CPPFLAGS)
if HAVE_WIN32
EXTRA_libheimdall_la_DEPENDENCIES += libheimdall_win32_rc.$(OBJEXT)
libheimdall_la_LDFLAGS += -Wl,libheimdall_win32_rc.$(OBJEXT) -avoid-version -Wc,-static
if HAVE_DYNAMIC_PLUGINS
libheimdall_la_CPPFLAGS += -DLIBHEIMDALL_DLL_EXPORT
endif
endif
if HAVE_OS2
libheimdall_la_LDFLAGS += -avoid-version
endif
if HAVE_DARWIN
libheimdall_la_LDFLAGS += -Xlinker -install_name -Xlinker @rpath/libheimdall.dylib
endif

# iOS and tvOS applications cannot install global shared libraries and
# dylibs must be in frameworks so there's no need for libtool versioning.
if HAVE_IOS
libheimdall_la_LDFLAGS += -avoid-version
endif
if HAVE_TVOS
libheimdall_la_LDFLAGS += -avoid-version
endif
if HAVE_ANDROID
libheimdall_la_LDFLAGS += -avoid-version
endif

pkgconfig_DATA = libheimdall.pc

libheimdall.pc: libheimdall.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="lib/$@"

libheimdall_win32_rc.$(OBJEXT): libheimdall_win32_rc.rc $(top_srcdir)/extras/package/win32/libheimdall.dll.manifest
	$(WINDRES) --include-dir $(top_srcdir)/share --include-dir $(top_srcdir)/extras/package/win32 -i $< -o $@

checkheader_verbose = $(checkheader_verbose_$(V))
checkheader_verbose_ = $(checkheader_verbose_$(AM_DEFAULT_VERBOSITY))
checkheader_verbose_0 = @echo "  CHECK    $(@:checkheader_%_h=heimdall/%.h)";
checkheader_verbose__0 = $(checkheader_verbose_0)
nodist_check_SCRIPTS = $(pkginclude_HEADERS:../include/heimdall/%.h=checkheader_%_h)
MOSTLYCLEANFILES = \
	$(pkginclude_HEADERS:../include/heimdall/%.h=checkheader_%_h.c) \
	$(pkginclude_HEADERS:../include/heimdall/%.h=checkheader_%_h.lo)

checkheader_%_h:
	@echo "#include <stdbool.h>" > $@.c
	@echo "#include <heimdall/libheimdall.h>" >> $@.c
	@echo "#include <heimdall/$*.h>" >> $@.c
	$(checkheader_verbose)$(LTCOMPILE) -c $@.c -o $@.lo

check-local:
	$(SHELL) $(top_srcdir)/src/check_headers $(pkginclude_HEADERS)
