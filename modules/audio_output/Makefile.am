aoutdir = $(pluginsdir)/audio_output
aout_PLUGINS =
aout_LTLIBRARIES =
aout_RPATH =

if HAVE_PARTIAL_LINKING
noinst_LTLIBRARIES += $(aout_PLUGINS)
else
aout_LTLIBRARIES += $(aout_PLUGINS)
aout_RPATH += -rpath '$(aoutdir)'
endif


# Install the pkglib/pkglibexec first before relinking the modules.
# This ensures libtool re-linking is not racy.
installaoutLTLIBRARIES = install-aoutLTLIBRARIES
$(installaoutLTLIBRARIES): install-pkglibLTLIBRARIES

libheimdall_android_audioformat_jni_la_SOURCES = \
	audio_output/android/audioformat_jni.c \
	audio_output/android/audioformat_jni.h

libheimdall_android_dynamicsprocessing_jni_la_SOURCES = \
	audio_output/android/dynamicsprocessing_jni.c \
	audio_output/android/dynamicsprocessing_jni.h

if HAVE_ANDROID
noinst_LTLIBRARIES += \
	libheimdall_android_audioformat_jni.la \
	libheimdall_android_dynamicsprocessing_jni.la
endif

libopensles_android_plugin_la_SOURCES = audio_output/android/opensles.c
libopensles_android_plugin_la_LIBADD = $(LIBDL) $(LIBM)

libandroid_audiotrack_plugin_la_SOURCES = audio_output/android/audiotrack.c audio_output/android/device.h
libandroid_audiotrack_plugin_la_LIBADD = libandroid_env.la \
	libheimdall_android_audioformat_jni.la \
	libheimdall_android_dynamicsprocessing_jni.la

libandroid_aaudio_plugin_la_SOURCES = audio_output/android/aaudio.c audio_output/android/device.h
libandroid_aaudio_plugin_la_LIBADD = libandroid_env.la libheimdall_android_dynamicsprocessing_jni.la $(LIBDL)

libandroid_audiodevice_plugin_la_SOURCES = audio_output/android/device.c audio_output/android/device.h
libandroid_audiodevice_plugin_la_LIBADD = libandroid_env.la \
	libheimdall_android_audioformat_jni.la \
	libheimdall_android_dynamicsprocessing_jni.la

if HAVE_ANDROID
aout_PLUGINS += libandroid_audiodevice_plugin.la libopensles_android_plugin.la \
	libandroid_audiotrack_plugin.la libandroid_aaudio_plugin.la
endif

libadummy_plugin_la_SOURCES = audio_output/adummy.c

libafile_plugin_la_SOURCES = audio_output/file.c
libafile_plugin_la_CPPFLAGS = $(AM_CPPFLAGS)

libamem_plugin_la_SOURCES = audio_output/amem.c

aout_PLUGINS += \
	libadummy_plugin.la \
	libafile_plugin.la \
	libamem_plugin.la

liboss_plugin_la_SOURCES = audio_output/oss.c audio_output/volume.h
liboss_plugin_la_LIBADD = $(OSS_LIBS) $(LIBM)
if HAVE_OSS
aout_PLUGINS += liboss_plugin.la
endif

libalsa_plugin_la_SOURCES = audio_output/alsa.c audio_output/volume.h
libalsa_plugin_la_CFLAGS = $(AM_CFLAGS) $(ALSA_CFLAGS)
libalsa_plugin_la_LIBADD = $(ALSA_LIBS) $(LIBM)
if HAVE_ALSA
aout_PLUGINS += libalsa_plugin.la
endif

libheimdall_pipewire_la_SOURCES = audio_output/heimdall_pipewire.c audio_output/heimdall_pipewire.h
libheimdall_pipewire_la_CFLAGS = $(AM_CFLAGS) $(PIPEWIRE_CFLAGS)
libheimdall_pipewire_la_LIBADD = $(PIPEWIRE_LIBS) $(LTLIBHEIMDALLCORE)
libheimdall_pipewire_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^heimdall_pw_ \
	-version-info 0:0:0
libaout_pipewire_plugin_la_SOURCES = audio_output/pipewire.c
libaout_pipewire_plugin_la_CFLAGS = $(AM_CFLAGS) $(PIPEWIRE_CFLAGS)
libaout_pipewire_plugin_la_LIBADD = libheimdall_pipewire.la $(PIPEWIRE_LIBS) $(LIBM)
if HAVE_PIPEWIRE
pkglib_LTLIBRARIES += libheimdall_pipewire.la
aout_PLUGINS += libaout_pipewire_plugin.la
endif

libheimdall_pulse_la_SOURCES = audio_output/heimdallpulse.c audio_output/heimdallpulse.h
libheimdall_pulse_la_CFLAGS = $(AM_CFLAGS) $(PULSE_CFLAGS)
libheimdall_pulse_la_LIBADD = $(PULSE_LIBS) $(LTLIBHEIMDALLCORE)
libheimdall_pulse_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^heimdall_pa_ \
	-version-info 0:0:0
libpulse_plugin_la_SOURCES = audio_output/pulse.c
libpulse_plugin_la_CFLAGS = $(AM_CFLAGS) $(PULSE_CFLAGS)
libpulse_plugin_la_LIBADD = libheimdall_pulse.la $(PULSE_LIBS) $(LIBM)
if HAVE_PULSE
pkglib_LTLIBRARIES += libheimdall_pulse.la
aout_PLUGINS += libpulse_plugin.la
endif

libjack_plugin_la_SOURCES = audio_output/jack.c audio_output/volume.h
libjack_plugin_la_CFLAGS = $(AM_CFLAGS) $(JACK_CFLAGS)
libjack_plugin_la_LIBADD = $(JACK_LIBS) $(LIBM)
if HAVE_JACK
aout_PLUGINS += libjack_plugin.la
endif

libmmdevice_plugin_la_SOURCES = audio_output/mmdevice.c audio_output/mmdevice.h
libmmdevice_plugin_la_LIBADD = $(LIBCOM) $(LIBM)
libwinstore_plugin_la_SOURCES = audio_output/winstore.c audio_output/mmdevice.h
libwinstore_plugin_la_LIBADD = $(LIBCOM) -lmmdevapi
libwasapi_plugin_la_SOURCES = audio_output/wasapi.c
libwasapi_plugin_la_LIBADD = $(LIBCOM) -lksuser
if HAVE_WASAPI
aout_PLUGINS += libwasapi_plugin.la
if !HAVE_WINSTORE
aout_PLUGINS += libmmdevice_plugin.la
else
aout_PLUGINS += libwinstore_plugin.la
endif
endif

libkai_plugin_la_SOURCES = audio_output/kai.c
libkai_plugin_la_LIBADD = $(KAI_LIBS)
if HAVE_KAI
aout_PLUGINS += libkai_plugin.la
endif

libsndio_plugin_la_SOURCES = audio_output/sndio.c
libsndio_plugin_la_LIBADD = -lsndio $(LIBM)
if HAVE_SNDIO
aout_PLUGINS += libsndio_plugin.la
endif

libwaveout_plugin_la_SOURCES = audio_output/waveout.c \
	audio_output/windows_audio_common.h
libwaveout_plugin_la_LIBADD = -lwinmm
if HAVE_WIN32_DESKTOP
aout_PLUGINS += libwaveout_plugin.la
endif

libavsamplebuffer_plugin_la_SOURCES = audio_output/apple/avsamplebuffer.m \
	audio_output/apple/channel_layout.c
if HAVE_IOS
libavsamplebuffer_plugin_la_SOURCES += audio_output/apple/avaudiosession_common.m
endif
if HAVE_TVOS
libavsamplebuffer_plugin_la_SOURCES += audio_output/apple/avaudiosession_common.m
endif
if HAVE_XROS
libavsamplebuffer_plugin_la_SOURCES += audio_output/apple/avaudiosession_common.m
endif
if HAVE_WATCHOS
libavsamplebuffer_plugin_la_SOURCES += audio_output/apple/avaudiosession_common.m
endif
libavsamplebuffer_plugin_la_LDFLAGS = $(AM_LDFLAGS) \
	-Wl,-framework,CoreMedia,-framework,Foundation,-framework,AVFoundation
libavsamplebuffer_plugin_la_OBJCFLAGS = $(AM_OBJCFLAGS) -fobjc-arc

libauhal_plugin_la_SOURCES = audio_output/apple/auhal.c \
	audio_output/apple/coreaudio_common.c audio_output/apple/coreaudio_common.h \
	audio_output/apple/channel_layout.c

libauhal_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(aout_RPATH) \
	-Wl,-framework,CoreAudio,-framework,AudioUnit,-framework,AudioToolbox,-framework,CoreServices
if HAVE_OSX
aout_PLUGINS += libauhal_plugin.la libavsamplebuffer_plugin.la
endif
libaudiounit_ios_plugin_la_SOURCES = audio_output/apple/audiounit_ios.m \
	audio_output/apple/coreaudio_common.c audio_output/apple/coreaudio_common.h \
	audio_output/apple/channel_layout.c audio_output/apple/avaudiosession_common.m
libaudiounit_ios_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(aout_RPATH) \
	-Wl,-framework,Foundation,-framework,CoreAudio,-framework,AudioToolbox,-framework,UIKit,-framework,AVFoundation
if HAVE_IOS_OR_TVOS
aout_PLUGINS += libaudiounit_ios_plugin.la libavsamplebuffer_plugin.la
endif
if HAVE_XROS
aout_PLUGINS += libaudiounit_ios_plugin.la libavsamplebuffer_plugin.la
endif
if HAVE_WATCHOS
aout_PLUGINS += libavsamplebuffer_plugin.la
endif
