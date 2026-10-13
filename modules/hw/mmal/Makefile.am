mmaldir = $(pluginsdir)/mmal
mmal_LTLIBRARIES =
mmal_PLUGINS =

if HAVE_PARTIAL_LINKING
noinst_LTLIBRARIES += $(mmal_PLUGINS)
else
mmal_LTLIBRARIES += $(mmal_PLUGINS)
endif

libheimdall_mmal_la_SOURCES = hw/mmal/subpic.c hw/mmal/subpic.h \
	hw/mmal/mmal_picture.c hw/mmal/mmal_picture.h \
	hw/mmal/mmal_cma.c hw/mmal/mmal_cma.h
libheimdall_mmal_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libheimdall_mmal_la_LIBADD = $(MMAL_LIBS)

libmmal_vout_plugin_la_SOURCES = hw/mmal/vout.c
libmmal_vout_plugin_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libmmal_vout_plugin_la_LIBADD = $(LIBM) libheimdall_mmal.la

libmmal_codec_plugin_la_SOURCES = hw/mmal/codec.c
libmmal_codec_plugin_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libmmal_codec_plugin_la_LIBADD = libheimdall_mmal.la

libmmal_converter_plugin_la_SOURCES = hw/mmal/converter.c
libmmal_converter_plugin_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libmmal_converter_plugin_la_LIBADD = libheimdall_mmal.la

libmmal_deinterlace_plugin_la_SOURCES = hw/mmal/deinterlace.c
libmmal_deinterlace_plugin_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libmmal_deinterlace_plugin_la_LIBADD = libheimdall_mmal.la

libmmal_device_plugin_la_SOURCES = hw/mmal/decoder_device.c
libmmal_device_plugin_la_CFLAGS = $(AM_CFLAGS) $(MMAL_CFLAGS)
libmmal_device_plugin_la_LIBADD = libheimdall_mmal.la

if HAVE_MMAL
noinst_LTLIBRARIES += libheimdall_mmal.la
mmal_PLUGINS += \
	libmmal_device_plugin.la \
	libmmal_deinterlace_plugin.la \
	libmmal_converter_plugin.la \
	libmmal_codec_plugin.la \
	libmmal_vout_plugin.la
endif
