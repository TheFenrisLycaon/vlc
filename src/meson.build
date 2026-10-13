#
# Heimdall revision file generation
#
git_dir = heimdall_src_root / '.git'
rev_target = vcs_tag(command: ['git',
    '--git-dir', git_dir,
    'describe', '--tags', '--long',
    '--match', '?.*.*', '--always'],
    input: 'revision.c.in',
    output: 'revision.c')

#
# FourCC preprocessor
#

fourcc_gen = executable('fourcc_gen',
    ['misc/fourcc_gen.c'],
    include_directories: heimdall_include_dirs,
    native: true
)

fourcc = custom_target('fourcc_tables.h',
                        output:  ['fourcc_tables.h'],
                        depend_files: ['misc/fourcc_list.h', '../include/heimdall_fourcc.h'],
                        capture: true,
                        command: [fourcc_gen])


meson_build_config = custom_target('build_config.c',
    command: [find_program('../buildsystem/gen-build-config.py'),
              '--',
              meson.build_options().replace('\'"\'"\'', '\\\'')],
    capture: true,
    output: 'build_config.c',
)

#
# libheimdallcore dependencies
#
anl_lib = cc.find_library('anl', required: false)

libheimdallcore_deps = [
    m_lib, dl_lib, threads_dep, intl_dep, socket_libs, iconv_dep, anl_lib, idn_dep
]

heimdallcore_cargs = [ '-DMODULE_STRING="core"', '-DHAVE_DYNAMIC_PLUGINS' ]
libheimdallcore_link_args = []

if host_system == 'darwin'
    libheimdallcore_deps += foundation_dep
    libheimdallcore_deps += corefoundation_dep
    libheimdallcore_deps += declare_dependency(link_args: ['-framework', 'CFNetwork'])
    libheimdallcore_link_args += [
        '-Wl,-U,_heimdall_static_modules',
        '-Wl,-undefined,dynamic_lookup',
    ]
    heimdallcore_cargs += [
        '-DHAVE_DYLIB_DYNAMIC_LOOKUP=1',
    ]
elif host_system == 'windows'
min_windows_version_test = '''
#include <sdkddkver.h>
#if !defined(@0@)
# error SDK too old for @0@
#elif _WIN32_WINNT < @0@
# error building for older version than @0@
#endif
'''

    libheimdallcore_deps += cc.find_library('bcrypt')
    libheimdallcore_deps += cc.find_library('normaliz')
    if not have_win_store
        libheimdallcore_deps += cc.find_library('winmm')
    else
        libheimdallcore_deps += cc.find_library('runtimeobject')
    endif
    if cc.compiles(min_windows_version_test.format('_WIN32_WINNT_WIN8'), name: 'Compiling for Win8+')
        libheimdallcore_deps += cc.find_library('synchronization')
    endif
    heimdallcore_cargs += ['-DHEIMDALL_DLL_EXPORT']
elif host_system == 'os/2'
    heimdallcore_cargs += ['-DHEIMDALL_DLL_EXPORT']
endif

#
# libheimdallcore library
#

libheimdallcore_sources_base = files(
    'libheimdall.c',
    'libheimdall.h',
    'libheimdall-module.c',
    'missing.c',
    'version.c',
    'config/cat.c',
    'config/configuration.h',
    'config/core.c',
    'config/chain.c',
    'config/dirs.c',
    'config/file.c',
    'config/help.c',
    'config/intf.c',
    'config/cmdline.c',
    'config/getopt.c',
    'config/heimdall_getopt.h',
    'config/jaro_winkler.c',
    'extras/libc.c',
    'media_source/media_source.c',
    'media_source/media_source.h',
    'media_source/media_tree.c',
    'modules/modules.h',
    'modules/modules.c',
    'modules/bank.c',
    'modules/cache.c',
    'modules/entry.c',
    'modules/textdomain.c',
    'interface/dialog.c',
    'interface/interface.c',
    'playlist/content.c',
    'playlist/content.h',
    'playlist/control.c',
    'playlist/control.h',
    'playlist/export.c',
    'playlist/item.c',
    'playlist/item.h',
    'playlist/notify.c',
    'playlist/notify.h',
    'playlist/player.c',
    'playlist/player.h',
    'playlist/playlist.c',
    'playlist/playlist.h',
    'playlist/preparse.c',
    'playlist/preparse.h',
    'playlist/randomizer.c',
    'playlist/randomizer.h',
    'playlist/request.c',
    'playlist/shuffle.c',
    'playlist/sort.c',
    'preparser/art.c',
    'preparser/art.h',
    'preparser/external.c',
    'preparser/fetcher.c',
    'preparser/fetcher.h',
    'preparser/ipc.c',
    'preparser/internal.c',
    'preparser/preparser.c',
    'preparser/preparser.h',
    'input/item.c',
    'input/access.c',
    'clock/clock_internal.c',
    'clock/input_clock.c',
    'clock/clock.c',
    'input/decoder.c',
    'input/decoder_prevframe.c',
    'input/decoder_device.c',
    'input/decoder_helpers.c',
    'input/demux.c',
    'input/demux_chained.c',
    'input/es_out.c',
    'input/es_out_source.c',
    'input/es_out_timeshift.c',
    'input/input.c',
    'input/info.h',
    'input/meta.c',
    'input/parse.c',
    'input/attachment.c',
    'input/source.c',
    'input/source.h',
    'player/player.c',
    'player/player.h',
    'player/input.c',
    'player/timer.c',
    'player/track.c',
    'player/title.c',
    'player/aout.c',
    'player/vout.c',
    'player/osd.c',
    'player/medialib.c',
    'player/metadata.c',
    'clock/input_clock.h',
    'clock/clock.h',
    'clock/clock_internal.h',
    'input/decoder.h',
    'input/decoder_prevframe.h',
    'input/demux.h',
    'input/es_out.h',
    'input/event.h',
    'input/item.h',
    'input/mrl_helpers.h',
    'input/stream.h',
    'input/input_internal.h',
    'input/input_interface.h',
    'input/vlm_internal.h',
    'input/vlm_event.h',
    'input/replay_gain.c',
    'input/resource.h',
    'input/resource.c',
    'input/services_discovery.c',
    'input/stats.c',
    'input/stream.c',
    'input/stream_fifo.c',
    'input/stream_extractor.c',
    'input/stream_filter.c',
    'input/stream_memory.c',
    'input/subtitles.c',
    'input/var.c',
    'audio_output/aout_internal.h',
    'audio_output/common.c',
    'audio_output/dec.c',
    'audio_output/filters.c',
    'audio_output/meter.c',
    'audio_output/output.c',
    'audio_output/volume.c',
    'video_output/chrono.h',
    'video_output/control.c',
    'video_output/control.h',
    'video_output/display.c',
    'video_output/display.h',
    'video_output/inhibit.c',
    'video_output/inhibit.h',
    'video_output/interlacing.c',
    'video_output/snapshot.c',
    'video_output/snapshot.h',
    'video_output/statistic.h',
    'video_output/video_output.c',
    'video_output/video_text.c',
    'video_output/video_epg.c',
    'video_output/video_widgets.c',
    'video_output/vout_subpictures.c',
    'video_output/vout_spuregion_helper.h',
    'video_output/vout_wrapper.h',
    'video_output/video_window.c',
    'video_output/video_window.h',
    'video_output/opengl.c',
    'video_output/vout_intf.c',
    'video_output/vout_internal.h',
    'video_output/vout_private.h',
    'video_output/vout_wrapper.c',
    'video_output/window.c',
    'network/getaddrinfo.c',
    'network/http_auth.c',
    'network/httpd.c',
    'network/io.c',
    'network/udp.c',
    'network/rootbind.c',
    'network/stream.c',
    'network/tls.c',
    'text/charset.c',
    'text/memstream.c',
    'text/strings.c',
    'text/unicode.c',
    'text/url.c',
    'text/filesystem.c',
    'text/iso_lang.c',
    'text/iso-639_def.h',
    'misc/actions.c',
    'misc/ancillary.c',
    'misc/chroma_probe.c',
    'misc/executor.c',
    'misc/md5.c',
    'misc/probe.c',
    'misc/rand.c',
    'misc/mtime.c',
    'misc/frame.c',
    'misc/fifo.c',
    'misc/filesystem.c',
    'misc/fourcc.c',
    'misc/fourcc_list.h',
    'misc/es_format.c',
    'misc/extensions.c',
    'misc/picture.c',
    'misc/picture.h',
    'misc/picture_fifo.c',
    'misc/picture_pool.c',
    'misc/interrupt.h',
    'misc/interrupt.c',
    'misc/keystore.c',
    'misc/renderer_discovery.c',
    'misc/threads.c',
    'misc/cpu.c',
    'misc/diffutil.c',
    'misc/epg.c',
    'misc/exit.c',
    'misc/image.c',
    'misc/messages.c',
    'misc/mime.c',
    'misc/objects.c',
    'misc/objres.c',
    'misc/queue.c',
    'misc/variables.h',
    'misc/variables.c',
    'misc/xml.c',
    'misc/addons.c',
    'misc/filter.c',
    'misc/filter_chain.c',
    'misc/httpcookies.c',
    'misc/fingerprinter.c',
    'misc/text_style.c',
    'misc/sort.c',
    'misc/subpicture.c',
    'misc/subpicture.h',
    'misc/medialibrary.c',
    'misc/viewpoint.c',
    'misc/rcu.c',
    'misc/tracer.c',
)

libheimdallcore_sources_base += meson_build_config

libheimdallcore_sout_sources = [
    'stream_output/sap.c',
    'stream_output/stream_output.c',
    'stream_output/stream_output.h'
]

libheimdallcore_vlm_sources = [
    'input/vlm.c',
    'input/vlm_event.c',
    'input/vlmshell.c'
]

libheimdallcore_sources = [
    libheimdallcore_sources_base,
    libheimdallcore_sout_sources,
]

if get_option('lycaria_manager')
    libheimdallcore_sources += libheimdallcore_vlm_sources
endif

if host_system == 'darwin'
    libheimdallcore_sources += [
        'darwin/dirs.m',
        'darwin/error.c',
        'darwin/netconf.m',
        'darwin/specific.c',

        'posix/filesystem.c',
        'posix/plugin.c',
        'posix/rand.c',
        'posix/timer.c',
        'posix/sort.c',
        'posix/thread.c',
        'posix/wait.c',
    ]
    if have_osx
        cdata.set('HAVE_HEIMDALL_PROCESS_SPAWN', 1)
        cdata.set('HAVE_HEIMDALL_EXTERNAL_PREPARSER', 1)
        libheimdallcore_sources += [
            'posix/spawn.c',
            'posix/process.c',
        ]
    endif
elif host_system == 'windows'
    cdata.set('HAVE_HEIMDALL_PROCESS_SPAWN', 1)
    cdata.set('HAVE_HEIMDALL_EXTERNAL_PREPARSER', 1)
    libheimdallcore_sources += [
        'win32/dirs-common.c',
        'win32/error.c',
        'win32/filesystem.c',
        'win32/netconf.c',
        'win32/plugin.c',
        'win32/rand.c',
        'win32/specific.c',
        'win32/thread.c',
        'win32/process.c',
    ]

    if have_win_store
        libheimdallcore_sources += [
            'posix/timer.c',
            'win32/dirs-uap.c',
        ]
    else
        libheimdallcore_sources += [
            'win32/timer.c',
            'win32/dirs.c',
            'win32/spawn.c',
        ]
    endif
elif host_system == 'os/2'
    libheimdallcore_sources += [
        'os2/dirs.c',
        'darwin/error.c',
        'os2/filesystem.c',
        'os2/getaddrinfo.c',
        'os2/netconf.c',
        'os2/plugin.c',
        'os2/specific.c',
        'os2/rand.c',
        'os2/thread.c',
    ]
else
    libheimdallcore_sources += [
        'posix/filesystem.c',
        'posix/plugin.c',
        'posix/rand.c',
        'posix/timer.c',
    ]

    if host_system != 'android'
        cdata.set('HAVE_HEIMDALL_PROCESS_SPAWN', 1)
        cdata.set('HAVE_HEIMDALL_EXTERNAL_PREPARSER', 1)
        libheimdallcore_sources += [
            'posix/dirs.c',
            'posix/error.c',
            'posix/netconf.c',
            'posix/spawn.c',
            'posix/picture.c',
            'posix/specific.c',
            'posix/thread.c',
            'posix/process.c',
        ]
    endif

    if cdata.get('HAVE_QSORT_R', 0) == 1
        libheimdallcore_sources += 'posix/sort.c'
    endif

    if host_system not in ['android', 'linux', 'freebsd']
        libheimdallcore_sources += 'posix/wait.c'
    endif

    if host_system == 'android'
        libheimdallcore_sources += [
            'android/error.c',
            'android/specific.c',
            'android/thread.c',

            'linux/cpu.c',
            'linux/dirs.c',
            'linux/thread.c',
        ]
    endif

    if host_system == 'linux'
        libheimdallcore_sources += [
            'linux/cpu.c',
            'linux/dirs.c',
            'linux/filesystem.c',
            'linux/thread.c',
        ]
    endif

    if host_system == 'freebsd'
        libheimdallcore_sources += [
            'freebsd/cpu.c',
            'freebsd/thread.c',
        ]
    endif

    if host_system == 'netbsd'
        libheimdallcore_sources += [
            'netbsd/thread.c',
        ]
    endif

    if host_system == 'openbsd'
        libheimdallcore_sources += [
            'openbsd/cpu.c',
            'openbsd/thread.c',
        ]
    endif

    if host_system != 'android'
        if anl_lib.found()
            libheimdallcore_sources += 'linux/getaddrinfo.c'
        else
            libheimdallcore_sources += 'posix/getaddrinfo.c'
        endif
    endif
endif

if get_option('update-check').allowed()
    if not gcrypt_dep.found()
        error('libgcrypt is required for update checking system')
    endif
    libheimdallcore_sources += [ 'misc/update.c', 'misc/update_crypto.c' ]
    heimdallcore_cargs += '-DUPDATE_CHECK'
    libheimdallcore_deps += gcrypt_dep
endif

if host_system == 'windows'
    libheimdallcore_rc = configure_file(
        input: 'libheimdallcore_win32_rc.rc.in',
        output: 'libheimdallcore_win32_rc.rc',
        configuration: rc_conf,
        install: false,
    )
    libheimdallcore_sources += windows_rc.compile_resources(
        libheimdallcore_rc,
        include_directories: [
            heimdall_src_root / 'share',
            heimdall_src_root / 'extras/package/win32',
        ],
        depend_files: files('../extras/package/win32/libheimdallcore.dll.manifest'),
    )
endif

libheimdallcore_kwargs = {}
if heimdall_lib_versioning
    libheimdallcore_kwargs += {'version': '9.0.0'}
endif

libheimdallcore = library(
    'heimdallcore',
    libheimdallcore_sources, heimdall_about, fourcc, rev_target,
    include_directories: heimdall_include_dirs,
    c_args: heimdallcore_cargs,
    link_args: libheimdallcore_link_args,
    link_with: [heimdall_libcompat],
    link_args: gc_sections_linkargs,
    dependencies: libheimdallcore_deps,
    install: true,
    install_dir: libdir_path,
    kwargs: libheimdallcore_kwargs,
)

libheimdallcore_dep = declare_dependency(link_with: libheimdallcore, sources: heimdall_about)

heimdall_tests += {
    'name' : 'input_es_out',
    'sources' : files(
        'input/test/es_out.c',
        'input/es_out.c',
        'input/es_out.h',
        'input/source.c',
        'input/source.h',
        'input/item.c',
        'input/item.h',
        'clock/clock.c',
        'clock/clock.h',
        'text/strings.c',
        'clock/clock_internal.c',
        'clock/clock_internal.h',
        'clock/input_clock.c',
        'clock/input_clock.h',
  ),
  'suite' : ['src'],
  'link_with' : [libheimdallcore],
  'include_directories' : [include_directories('.')],
}

heimdall_tests += {
    'name' : 'input_clock',
    'sources' : files('clock/test/input_clock.c',
        'clock/input_clock.c',
        'clock/input_clock.h',
        'clock/clock_internal.c',
        'clock/clock_internal.h',
        'clock/clock.c',
        'clock/clock.h',
    ),
    'suite' : ['src'],
    'link_with' : [libheimdallcore],
}

subdir('rust')
