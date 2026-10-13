###############################################################################
# Automake targets and declarations
###############################################################################

AUTOMAKE_OPTIONS = subdir-objects

NULL =
EXTRA_DIST = \
	heimdall-plugin.pc.in \
	revision.txt

BUILT_SOURCES = $(nodist_pluginsinclude_HEADERS)
CLEANFILES = $(BUILT_SOURCES)

SUFFIXES = .pc.in .pc .rc.in .rc
TEST_EXTENSIONS =

###############################################################################
# Headers
###############################################################################

pluginsincludedir = $(pkgincludedir)/plugins

pluginsinclude_HEADERS.h = \
	../include/heimdall_access.h \
	../include/heimdall_actions.h \
	../include/heimdall_addons.h \
	../include/heimdall_ancillary.h \
	../include/heimdall_aout.h \
	../include/heimdall_aout_volume.h \
	../include/heimdall_arrays.h \
	../include/heimdall_atomic.h \
	../include/heimdall_avcodec.h \
	../include/heimdall_bits.h \
	../include/heimdall_block.h \
	../include/heimdall_block_helper.h \
	../include/heimdall_boxes.h \
	../include/heimdall_charset.h \
	../include/heimdall_codec.h \
	../include/heimdall_common.h \
	../include/heimdall_config.h \
	../include/heimdall_config_cat.h \
	../include/heimdall_configuration.h \
	../include/heimdall_cpu.h \
	../include/heimdall_clock.h \
	../include/heimdall_chroma_probe.h \
	../include/heimdall_decoder.h \
	../include/heimdall_demux.h \
	../include/heimdall_dialog.h \
	../include/heimdall_diffutil.h \
	../include/heimdall_epg.h \
	../include/heimdall_es.h \
	../include/heimdall_es_out.h \
	../include/heimdall_executor.h \
	../include/heimdall_filter.h \
	../include/heimdall_fingerprinter.h \
	../include/heimdall_fourcc.h \
	../include/heimdall_frame.h \
	../include/heimdall_fs.h \
	../include/heimdall_gcrypt.h \
	../include/heimdall_hash.h \
	../include/heimdall_http.h \
	../include/heimdall_httpd.h \
	../include/heimdall_image.h \
	../include/heimdall_inhibit.h \
	../include/heimdall_input.h \
	../include/heimdall_input_item.h \
	../include/heimdall_interface.h \
	../include/heimdall_interrupt.h \
	../include/heimdall_keystore.h \
	../include/heimdall_list.h \
	../include/heimdall_media_library.h \
	../include/heimdall_media_source.h \
	../include/heimdall_memstream.h \
	../include/heimdall_messages.h \
	../include/heimdall_tracer.h \
	../include/heimdall_meta.h \
	../include/heimdall_meta_fetcher.h \
	../include/heimdall_mime.h \
	../include/heimdall_modules.h \
	../include/heimdall_mouse.h \
	../include/heimdall_network.h \
	../include/heimdall_objects.h \
	../include/heimdall_opengl.h \
	../include/heimdall_opengl_interop.h \
	../include/heimdall_opengl_filter.h \
	../include/heimdall_opengl_picture.h \
	../include/heimdall_opengl_sampler.h \
	../include/heimdall_picture.h \
	../include/heimdall_picture_fifo.h \
	../include/heimdall_picture_pool.h \
	../include/heimdall_player.h \
	../include/heimdall_playlist.h \
	../include/heimdall_playlist_export.h \
	../include/heimdall_plugin.h \
	../include/heimdall_poll.h \
	../include/heimdall_probe.h \
	../include/heimdall_preparser.h \
	../include/heimdall_process.h \
	../include/heimdall_preparser_ipc.h \
	../include/heimdall_queue.h \
	../include/heimdall_rand.h \
	../include/heimdall_renderer_discovery.h \
	../include/heimdall_replay_gain.h \
	../include/heimdall_services_discovery.h \
	../include/heimdall_sort.h \
	../include/heimdall_sout.h \
	../include/heimdall_spawn.h \
	../include/heimdall_spu.h \
	../include/heimdall_stream.h \
	../include/heimdall_stream_extractor.h \
	../include/heimdall_strings.h \
	../include/heimdall_subpicture.h \
	../include/heimdall_text_style.h \
	../include/heimdall_threads.h \
	../include/heimdall_tick.h \
	../include/heimdall_timestamp_helper.h \
	../include/heimdall_tls.h \
	../include/heimdall_url.h \
	../include/heimdall_variables.h \
	../include/heimdall_vector.h \
	../include/heimdall_video_splitter.h \
	../include/heimdall_viewpoint.h \
	../include/heimdall_vlm.h \
	../include/heimdall_vout.h \
	../include/heimdall_vout_display.h \
	../include/heimdall_vout_osd.h \
	../include/heimdall_window.h \
	../include/heimdall_xlib.h \
	../include/heimdall_xml.h \
	$(NULL)

pluginsinclude_HEADERS.hpp = \
	../include/heimdall_cxx_helpers.hpp \
	../include/heimdall_list.hpp \
	$(NULL)

pluginsinclude_HEADERS = \
	$(pluginsinclude_HEADERS.h) \
	$(pluginsinclude_HEADERS.hpp)
nodist_pluginsinclude_HEADERS = ../include/heimdall_about.h

noinst_HEADERS = \
	../include/heimdall_codecs.h \
	../include/heimdall_extensions.h \
	../include/heimdall_fixups.h \
	../include/heimdall_intf_strings.h \
	../include/heimdall_iso_lang.h \
	../include/heimdall_pgpkey.h \
	../include/heimdall_update.h \
	$(NULL)

../include/heimdall_about.h: Makefile.am $(top_srcdir)/COPYING $(top_srcdir)/THANKS $(top_srcdir)/AUTHORS
	$(AM_V_at)rm -f -- "$@.tmp"
	$(AM_V_at)$(MKDIR_P) "../include"
	$(AM_V_GEN)(echo "/* Automatically generated file - DO NOT EDIT */" && \
	echo "static const char psz_license[] =" && \
	sed 's/"/\\"/g;s/^.*$$/\"&\\n\"/' "$(top_srcdir)/COPYING" && \
	echo ";" && \
	echo "static const char psz_thanks[] =" && \
	sed '/\$$Id:/d;s/"/\\"/g;s/<.*.> //;s/^.*$$/\"&\\n\"/' \
		"$(top_srcdir)/THANKS" && \
	echo ";" && \
	echo "static const char psz_authors[] =" && \
	sed '/\$$Id:/d;s/"/\\"/g;s/<.*.> //;s/^.*$$/\"&\\n\"/' \
		"$(top_srcdir)/AUTHORS" && \
	echo ";") >> "$@.tmp"
	$(AM_V_at)mv -f -- "$@.tmp" "$@"

###############################################################################
# pkg-config integration
###############################################################################

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = heimdall-plugin.pc
CLEANFILES += $(pkgconfig_DATA)

heimdall-plugin.pc: heimdall-plugin.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="src/$@"

##############################################################################
# Windows resource files
##############################################################################

if HAVE_WIN32
noinst_DATA = libheimdallcore_win32_rc.rc
endif
EXTRA_DIST += libheimdallcore_win32_rc.rc.in

libheimdallcore_win32_rc.rc: libheimdallcore_win32_rc.rc.in $(top_builddir)/config.status
	$(AM_V_GEN)cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="src/$@"

###############################################################################
# Building libheimdall
###############################################################################

lib_LTLIBRARIES = libheimdallcore.la

AM_CPPFLAGS = $(INCICONV) $(IDN_CFLAGS) \
	-DMODULE_STRING=\"core\" \
	-DSYSDATADIR=\"$(datadir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPKGLIBDIR=\"$(pkglibdir)\" \
	-DPKGLIBEXECDIR=\"$(pkglibexecdir)\"
AM_CFLAGS = $(CFLAGS_libheimdallcore)
if HAVE_DYNAMIC_PLUGINS
AM_CPPFLAGS += -DHAVE_DYNAMIC_PLUGINS
if HAVE_WIN32
AM_CPPFLAGS += -DHEIMDALL_DLL_EXPORT
endif
endif
if HAVE_DBUS
AM_CPPFLAGS += -DHAVE_DBUS
AM_CFLAGS += $(DBUS_CFLAGS)
endif

libheimdallcore_la_SOURCES = \
	libheimdall.c \
	libheimdall.h \
	libheimdall-module.c \
	missing.c \
	revision.c \
	version.c \
	config/ansi_term.h \
	config/configuration.h \
	config/cat.c \
	config/core.c \
	config/chain.c \
	config/dirs.c \
	config/file.c \
	config/help.c \
	config/intf.c \
	config/cmdline.c \
	config/getopt.c \
	config/heimdall_getopt.h \
	config/jaro_winkler.c \
	config/heimdall_jaro_winkler.h \
	extras/libc.c \
	media_source/media_source.c \
	media_source/media_source.h \
	media_source/media_tree.c \
	modules/modules.h \
	modules/modules.c \
	modules/bank.c \
	modules/cache.c \
	modules/entry.c \
	modules/textdomain.c \
	interface/dialog.c \
	interface/interface.c \
	playlist/content.c \
	playlist/content.h \
	playlist/control.c \
	playlist/control.h \
	playlist/export.c \
	playlist/item.c \
	playlist/item.h \
	playlist/notify.c \
	playlist/notify.h \
	playlist/player.c \
	playlist/player.h \
	playlist/playlist.c \
	playlist/playlist.h \
	playlist/preparse.c \
	playlist/preparse.h \
	playlist/randomizer.c \
	playlist/randomizer.h \
	playlist/request.c \
	playlist/shuffle.c \
	playlist/sort.c \
	preparser/art.c \
	preparser/art.h \
	preparser/fetcher.c \
	preparser/fetcher.h \
	preparser/ipc.c \
	preparser/internal.c \
	preparser/preparser.c \
	preparser/preparser.h \
	preparser/external.c \
	input/item.c \
	input/access.c \
	clock/clock_internal.c \
	clock/input_clock.c \
	clock/clock.c \
	input/decoder.c \
	input/decoder_prevframe.c \
	input/decoder_device.c \
	input/decoder_helpers.c \
	input/demux.c \
	input/demux_chained.c \
	input/es_out.c \
	input/es_out_source.c \
	input/es_out_timeshift.c \
	input/input.c \
	input/info.h \
	input/meta.c \
	input/attachment.c \
	input/parse.c \
	input/replay_gain.c \
	player/player.c \
	player/player.h \
	player/input.c \
	player/timer.c \
	player/track.c \
	player/title.c \
	player/aout.c \
	player/vout.c \
	player/osd.c \
	player/medialib.c \
	player/metadata.c \
	clock/input_clock.h \
	clock/clock.h \
	clock/clock_internal.h \
	input/decoder.h \
	input/decoder_prevframe.h \
	input/demux.h \
	input/es_out.h \
	input/event.h \
	input/item.h \
	input/mrl_helpers.h \
	input/stream.h \
	input/input_internal.h \
	input/input_interface.h \
	input/vlm_internal.h \
	input/vlm_event.h \
	input/resource.h \
	input/resource.c \
	input/services_discovery.c \
	input/source.c \
	input/source.h \
	input/stats.c \
	input/stream.c \
	input/stream_fifo.c \
	input/stream_extractor.c \
	input/stream_filter.c \
	input/stream_memory.c \
	input/subtitles.c \
	input/var.c \
	audio_output/aout_internal.h \
	audio_output/common.c \
	audio_output/dec.c \
	audio_output/filters.c \
	audio_output/meter.c \
	audio_output/output.c \
	audio_output/volume.c \
	video_output/chrono.h \
	video_output/control.c \
	video_output/control.h \
	video_output/display.c \
	video_output/display.h \
	video_output/inhibit.c \
	video_output/inhibit.h \
	video_output/interlacing.c \
	video_output/snapshot.c \
	video_output/snapshot.h \
	video_output/statistic.h \
	video_output/video_output.c \
	video_output/video_text.c \
	video_output/video_epg.c \
	video_output/video_widgets.c \
	video_output/vout_subpictures.c \
	video_output/vout_spuregion_helper.h \
	video_output/vout_wrapper.h \
	video_output/video_window.c \
	video_output/video_window.h \
	video_output/window.c \
	video_output/opengl.c \
	video_output/vout_intf.c \
	video_output/vout_internal.h \
	video_output/vout_private.h \
	video_output/vout_wrapper.c \
	network/getaddrinfo.c \
	network/http_auth.c \
	network/httpd.c \
	network/io.c \
	network/udp.c \
	network/rootbind.c \
	network/stream.c \
	network/tls.c \
	text/charset.c \
	text/memstream.c \
	text/strings.c \
	text/unicode.c \
	text/url.c \
	text/filesystem.c \
	text/iso_lang.c \
	text/iso-639_def.h \
	misc/actions.c \
	misc/ancillary.c \
	misc/chroma_probe.c \
	misc/executor.c \
	misc/md5.c \
	misc/probe.c \
	misc/rand.c \
	misc/mtime.c \
	misc/frame.c \
	misc/fifo.c \
	misc/filesystem.c \
	misc/fourcc.c \
	misc/fourcc_list.h \
	misc/es_format.c \
	misc/extensions.c \
	misc/picture.c \
	misc/picture.h \
	misc/picture_fifo.c \
	misc/picture_pool.c \
	misc/interrupt.h \
	misc/interrupt.c \
	misc/keystore.c \
	misc/rcu.h \
	misc/rcu.c \
	misc/renderer_discovery.c \
	misc/threads.c \
	misc/threads.h \
	misc/cpu.c \
	misc/diffutil.c \
	misc/epg.c \
	misc/exit.c \
	misc/image.c \
	misc/messages.c \
	misc/tracer.c \
	misc/mime.c \
	misc/objects.c \
	misc/objres.c \
	misc/queue.c \
	misc/variables.h \
	misc/variables.c \
	misc/xml.c \
	misc/addons.c \
	misc/filter.c \
	misc/filter_chain.c \
	misc/httpcookies.c \
	misc/fingerprinter.c \
	misc/text_style.c \
	misc/sort.c \
	misc/subpicture.c \
	misc/subpicture.h \
	misc/medialibrary.c \
	misc/viewpoint.c
libheimdallcore_la_LIBADD = $(LIBS_libheimdallcore) \
	../compat/libcompat.la \
	$(LTLIBINTL) $(LTLIBICONV) \
	$(IDN_LIBS) $(SOCKET_LIBS) $(LIBRT) $(LIBDL) $(LIBM)

libheimdallcore_la_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS_libheimdallcore)

if HAVE_WIN32
libheimdallcore_la_SOURCES += \
	win32/dirs-common.c \
	win32/error.c \
	win32/filesystem.c \
	win32/netconf.c \
	win32/plugin.c \
	win32/rand.c \
	win32/specific.c \
	win32/thread.c \
	win32/process.c
if HAVE_WINSTORE
libheimdallcore_la_SOURCES += posix/timer.c win32/dirs-uap.c
else
libheimdallcore_la_SOURCES += win32/timer.c win32/dirs.c win32/spawn.c
libheimdallcore_la_LIBADD += -lwinmm -lole32 -luuid
endif
libheimdallcore_la_LIBADD += -lbcrypt
endif

if HAVE_OS2
libheimdallcore_la_SOURCES += \
	os2/dirs.c \
	darwin/error.c \
	os2/filesystem.c \
	os2/getaddrinfo.c \
	os2/netconf.c \
	os2/plugin.c \
	os2/specific.c \
	os2/rand.c \
	os2/thread.c
endif

if HAVE_EMSCRIPTEN
libheimdallcore_la_SOURCES += \
	emscripten/netconf.c \
	emscripten/thread.c
endif

if HAVE_DARWIN
libheimdallcore_la_SOURCES += \
	darwin/error.c \
	darwin/specific.c \
	darwin/thread.c
libheimdallcore_objc_la_SOURCES = \
	darwin/dirs.m \
	darwin/netconf.m
noinst_LTLIBRARIES = libheimdallcore_objc.la
endif

if HAVE_FREEBSD
libheimdallcore_la_SOURCES += \
	freebsd/cpu.c \
	freebsd/thread.c
endif

if HAVE_NETBSD
libheimdallcore_la_SOURCES += \
        netbsd/thread.c
endif

if HAVE_OPENBSD
libheimdallcore_la_SOURCES += \
        openbsd/cpu.c \
        openbsd/thread.c
endif

if HAVE_LINUX
libheimdallcore_la_SOURCES += \
	linux/cpu.c \
	linux/dirs.c \
	linux/thread.c
if !HAVE_ANDROID
libheimdallcore_la_SOURCES += \
	linux/filesystem.c
else
libheimdallcore_la_SOURCES += \
	android/error.c \
	android/specific.c \
	android/thread.c
endif
endif

if !HAVE_WIN32
if HAVE_PROCESS_SPAWN
libheimdallcore_la_SOURCES += posix/process.c
endif
if !HAVE_OS2
libheimdallcore_la_SOURCES += \
	posix/filesystem.c \
	posix/plugin.c \
	posix/rand.c \
	posix/timer.c
if !HAVE_LINUX
libheimdallcore_la_SOURCES += posix/wait.c
endif
if !HAVE_ANDROID
if HAVE_QSORT_R
libheimdallcore_la_SOURCES += posix/sort.c
endif
libheimdallcore_la_SOURCES += \
	posix/thread.c
libheimdallcore_la_LIBADD += $(LIBEXECINFO)
if HAVE_OSX
libheimdallcore_la_SOURCES += \
	posix/spawn.c
endif
if !HAVE_DARWIN
if !HAVE_EMSCRIPTEN
libheimdallcore_la_SOURCES += posix/netconf.c
endif
libheimdallcore_la_SOURCES += \
	posix/dirs.c \
	posix/error.c \
	posix/picture.c \
	posix/spawn.c \
	posix/specific.c
if HAVE_LIBANL
libheimdallcore_la_SOURCES += \
	linux/getaddrinfo.c
libheimdallcore_la_LIBADD += -lanl
else
libheimdallcore_la_SOURCES += \
	posix/getaddrinfo.c
endif
endif
endif
endif
endif

libheimdallcore_la_SOURCES += \
	stream_output/sap.c \
	stream_output/stream_output.c stream_output/stream_output.h
if ENABLE_VLM
libheimdallcore_la_SOURCES += input/vlm.c input/vlm_event.c input/vlmshell.c
endif

if UPDATE_CHECK
libheimdallcore_la_SOURCES += \
	misc/update.h misc/update.c \
	misc/update_crypto.c
AM_CPPFLAGS += -DUPDATE_CHECK
AM_CFLAGS += $(GCRYPT_CFLAGS)
libheimdallcore_la_LIBADD += $(GCRYPT_LIBS)
endif

libheimdallcore_la_LDFLAGS = \
	$(LDFLAGS_libheimdallcore) \
	-no-undefined \
	-export-symbols $(srcdir)/libheimdallcore.sym \
	-version-info 9:0:0
libheimdallcore_la_SOURCES += libheimdallcore.sym
EXTRA_libheimdallcore_la_DEPENDENCIES = libheimdallcore.sym

if HAVE_WIN32
EXTRA_libheimdallcore_la_DEPENDENCIES += libheimdallcore_win32_rc.$(OBJEXT)
libheimdallcore_la_LDFLAGS += -Wl,libheimdallcore_win32_rc.$(OBJEXT) -avoid-version -Wc,-static
endif
if HAVE_OS2
libheimdallcore_la_LDFLAGS += -avoid-version
endif
if HAVE_DBUS
libheimdallcore_la_LIBADD += $(DBUS_LIBS)
endif
if HAVE_DARWIN
libheimdallcore_objc_la_OBJCFLAGS = $(AM_OBJCFLAGS) -fobjc-arc
libheimdallcore_objc_la_LDFLAGS = -static
libheimdallcore_la_LIBADD += libheimdallcore_objc.la
libheimdallcore_la_LDFLAGS +=  -Wl,-framework,Foundation -Xlinker -install_name -Xlinker @rpath/libheimdallcore.dylib
endif

# iOS and tvOS applications cannot install global shared libraries and
# dylibs must be in frameworks so there's no need for libtool versioning.
if HAVE_IOS
libheimdallcore_la_LDFLAGS += -avoid-version
endif
if HAVE_TVOS
libheimdallcore_la_LDFLAGS += -avoid-version
endif
if HAVE_ANDROID
libheimdallcore_la_LDFLAGS += -avoid-version
endif

libheimdallcore_win32_rc.$(OBJEXT): libheimdallcore_win32_rc.rc $(top_srcdir)/extras/package/win32/libheimdallcore.dll.manifest
	$(WINDRES) --include-dir $(top_srcdir)/share --include-dir $(top_srcdir)/extras/package/win32 -i $< -o $@

# FourCC tables
BUILT_SOURCES += fourcc_tables.h
EXTRA_DIST += misc/fourcc_gen.c
MOSTLYCLEANFILES = fourcc_gen$(BUILDEXEEXT)

fourcc_gen$(BUILDEXEEXT): misc/fourcc_gen.c misc/fourcc_list.h ../include/heimdall_fourcc.h
	$(AM_V_at)rm -f -- $@
	$(AM_V_CC)$(BUILDCC) -I$(srcdir) -o $@ $<

fourcc_tables.h: fourcc_gen$(BUILDEXEEXT)
	$(AM_V_at)rm -f -- $@.tmp
	$(AM_V_GEN)$(builddir)/fourcc_gen$(BUILDEXEEXT) > $@.tmp
	$(AM_V_at)mv -f -- $@.tmp $@

# Unit/regression tests
#
check_PROGRAMS = \
	test_block \
	test_dictionary \
	test_executor \
	test_i18n_atof \
	test_interrupt \
	test_jaro_winkler \
	test_list \
	test_md5 \
	test_objects_cxx \
	test_picture_pool \
	test_sort \
	test_timer \
	test_url \
	test_utf8 \
	test_xmlent \
	test_headers \
	test_mrl_helpers \
	test_arrays \
	test_vector \
	test_shared_data_ptr \
	test_playlist \
	test_randomizer \
	test_media_source \
	test_extensions \
	test_thread \
	test_diffutil \
	test_cpp_list

TESTS = $(check_PROGRAMS) check_symbols

test_block_SOURCES = test/block_test.c
test_block_LDADD = $(LDADD) $(LIBS_libheimdallcore)
test_dictionary_SOURCES = test/dictionary.c
test_executor_SOURCES = test/executor.c
test_i18n_atof_SOURCES = test/i18n_atof.c
test_interrupt_SOURCES = test/interrupt.c
test_interrupt_LDADD = $(LDADD) $(LIBS_libheimdallcore)
test_jaro_winkler_SOURCES = test/jaro_winkler.c config/jaro_winkler.c
test_list_SOURCES = test/list.c
test_md5_SOURCES = test/md5.c
test_objects_cxx_SOURCES = misc/objects_cxx_test.cpp misc/objects.c
test_picture_pool_SOURCES = test/picture_pool.c
test_sort_SOURCES = test/sort.c
test_timer_SOURCES = test/timer.c
test_url_SOURCES = test/url.c
test_utf8_SOURCES = test/utf8.c
test_xmlent_SOURCES = test/xmlent.c
test_headers_SOURCES = test/headers.c
test_mrl_helpers_SOURCES = test/mrl_helpers.c
test_arrays_SOURCES = test/arrays.c
test_vector_SOURCES = test/vector.c
test_shared_data_ptr_SOURCES = test/shared_data_ptr.cpp
test_extensions_SOURCES = test/extensions.c
test_diffutil_SOURCES = test/diffutil.c
test_playlist_SOURCES = playlist/test.c \
	playlist/content.c \
	playlist/control.c \
	playlist/item.c \
	playlist/notify.c \
	playlist/player.c \
	playlist/playlist.c \
	playlist/preparse.c \
	playlist/randomizer.c \
	playlist/request.c \
	playlist/shuffle.c \
	playlist/sort.c
test_playlist_CFLAGS = -DTEST_PLAYLIST
test_randomizer_SOURCES = playlist/randomizer.c
test_randomizer_CFLAGS = -DTEST_RANDOMIZER
test_media_source_LDADD = $(LDADD) $(LIBS_libheimdallcore)
test_media_source_CFLAGS = -DTEST_MEDIA_SOURCE
test_media_source_SOURCES = media_source/test.c \
	media_source/media_source.c \
	media_source/media_tree.c
test_thread_SOURCES = test/thread.c
test_cpp_list_SOURCES = test/list_cpp.cpp

test_input_es_out_SOURCES = input/test/es_out.c \
	input/es_out.c input/es_out.h \
	input/source.c input/source.h \
	input/item.c input/item.h \
	clock/clock.c clock/clock.h \
	text/strings.c \
	clock/clock_internal.c clock/clock_internal.h \
	clock/input_clock.c clock/input_clock.h
check_PROGRAMS += test_input_es_out

test_input_clock_SOURCES = clock/test/input_clock.c \
	clock/input_clock.c clock/input_clock.h \
	clock/clock_internal.c clock/clock_internal.h \
	clock/clock.c clock/clock.h
check_PROGRAMS += test_input_clock

LDADD = libheimdallcore.la \
	../compat/libcompat.la

###############################################################################
# GIT revision
###############################################################################

BUILT_SOURCES += stamp-revision
MAINTAINERCLEANFILES = $(srcdir)/revision.txt $(srcdir)/revision.c

$(srcdir)/revision.c: $(srcdir)/revision.txt
	$(AM_V_at)rm -f -- $@
	$(AM_V_GEN)echo "const char psz_heimdall_changeset[] = \"$$(cat $<)\";" \
		> $@

$(srcdir)/revision.txt:
	$(AM_V_at)$(MAKE) stamp-revision
	$(AM_V_GEN)touch $@

stamp-revision:
	$(AM_V_at)rm -f -- revision.tmp
	$(AM_V_GEN)if ! git \
			-C "$(top_srcdir)" describe \
			--tags --long --match '?.*.*' --always; then \
		cat $(srcdir)/revision.txt ; \
	fi > revision.tmp
	$(AM_V_at)if diff revision.tmp $(srcdir)/revision.txt >/dev/null 2>&1; then \
		rm -f -- revision.tmp; \
	else \
		mv -f -- revision.tmp $(srcdir)/revision.txt; \
	fi
#2>&1

###############################################################################
# Unit/regression test
###############################################################################

checkheader_verbose = $(checkheader_verbose_$(V))
checkheader_verbose_ = $(checkheader_verbose_$(AM_DEFAULT_VERBOSITY))
checkheader_verbose__0 = $(checkheader_verbose_0)

checkheaders.h   = $(pluginsinclude_HEADERS.h:../include/%=checkheader_%.check)
checkheaders.hpp = $(pluginsinclude_HEADERS.hpp:../include/%=checkheader_%.check)
checkheaders.all = $(checkheaders.h) $(checkheaders.hpp)

$(checkheaders.all): checkheader_verbose_0 = @echo "  CHECK    $(@:checkheader_%.check=%)";
$(checkheaders.all): skip_checkheader_verbose_0 = @echo "  CHECK    $(@:checkheader_%.check=%) (SKIPPED)";
#$(checkheaders.hpp): checkheader_verbose_0 = @echo "  CHECK    $(@:checkheader_%_hpp=%.hpp)";
#$(checkheaders.hpp): skip_checkheader_verbose_0 = @echo "  CHECK    $(@:checkheader_%_hpp=%.hpp) (SKIPPED)";

nodist_check_SCRIPTS = $(checkheaders.all)
MOSTLYCLEANFILES += \
	$(checkheaders.h:%=%.c) $(checkheaders.h:%=%.lo) \
	$(checkheaders.hpp:%=%.cpp) $(checkheaders.hpp:%=%.lo)

checkheaders: $(checkheaders.all)
checkheader_%.check:
	$(checkheader_verbose) \
	sourcefile="$@.$(CHECKHEADER_SOURCE_EXT)"; exec 3>$${sourcefile} && \
	echo "#include <stdbool.h>" >&3 && \
	echo "#include <heimdall_common.h>" >&3 && \
	echo "#include <$*>" >&3 && \
	$(CHECKCOMPILE) $(CHECK_CFLAGS) -c $${sourcefile} -o $@.lo
$(checkheaders.h): CHECKHEADER_SOURCE_EXT=c
$(checkheaders.h): CHECKCOMPILE=$(LTCOMPILE)
$(checkheaders.hpp): CHECKHEADER_SOURCE_EXT=cpp
$(checkheaders.hpp): CHECKCOMPILE=$(LTCXXCOMPILE)

# Some exceptions for headers depending on other libraries
checkheader_heimdall_gcrypt.h.check: CHECK_CFLAGS = "-Dgcry_check_version(x)="
checkheader_heimdall_xlib.h.check: CHECK_CFLAGS = $(X_CFLAGS)
checkheader_heimdall_list.hpp.check: CHECK_CFLAGS = "-Drestrict="

SKIPPED_CHECKHEADERS =
if !HAVE_X
SKIPPED_CHECKHEADERS += heimdall_xlib.h
endif

$(SKIPPED_CHECKHEADERS:%=checkheader_%.check): checkheader_verbose_0 = $(skip_checkheader_verbose_0)
$(SKIPPED_CHECKHEADERS:%=checkheader_%.check): CHECKCOMPILE = exit 0;

dist_check_SCRIPTS = check_headers check_symbols

check-local:
	for h in `echo $(pkginclude_HEADERS) | sed -e s,\.\./include/,,g`; \
	do \
		echo grep - "#include <$$h>" $(srcdir)/test/headers.c ; \
		if ! grep -- "#include <$$h>" $(srcdir)/test/headers.c ; \
		then \
			echo "Header $$h not included in test/headers.c!"; \
			exit 1; \
		fi ; \
	done
	$(SHELL) $(srcdir)/check_headers $(pluginsinclude_HEADERS)

include rust/Makefile.am

FORCE:
	@echo "Generated source cannot be phony. Go away." >&2
	@exit 1

.PHONY: FORCE
