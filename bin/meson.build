# Do we build the main Heimdall binary?
build_heimdall = get_option('heimdall')

if build_heimdall and (host_system != 'darwin' or have_osx) and not have_win_store and host_system != 'android'
    heimdall_sources = []

    heimdall_deps = [m_lib, dl_lib, threads_dep]

    if host_system == 'darwin'
        heimdall_sources += ['darwinheimdall.m']
        heimdall_deps += corefoundation_dep
        if have_osx
            heimdall_deps += dependency('Cocoa', required: true)
        endif
    elif host_system == 'windows'
        heimdall_sources += ['winheimdall.c']
        heimdall_deps += cc.find_library('ole32', required: true)
    else
        heimdall_sources += ['heimdall.c', 'override.c']
        if x11_dep.found()
            heimdall_deps += [x11_dep]
        endif
    endif

    heimdall_win_extra = []
    if host_system == 'windows'
        heimdall_rc = configure_file(
            input: 'heimdall_win32_rc.rc.in',
            output: 'heimdall_win32_rc.rc',
            configuration: rc_conf,
            install: false,
        )
        heimdall_win_extra = windows_rc.compile_resources(
            heimdall_rc,
            include_directories: [
                heimdall_src_root / 'share/icons',
                heimdall_src_root / 'extras/package/win32',
            ],
            depend_files: files('../extras/package/win32/heimdall.exe.manifest'),
        )
    endif

    executable('heimdall',
        heimdall_sources + heimdall_win_extra,
        link_with: [libheimdall, libheimdallcore],
        link_args: gc_sections_linkargs,
        include_directories: [heimdall_include_dirs],
        dependencies: heimdall_deps,
        install: true,
        install_dir: bindir_path,
        win_subsystem: 'windows'
    )

    heimdall_top_builddir_def = '-DTOP_BUILDDIR="@0@"'.format(heimdall_build_root)
    heimdall_top_srcdir_def = '-DTOP_SRCDIR="@0@"'.format(heimdall_src_root)

    executable('heimdall-static',
        heimdall_sources,
        link_with: [libheimdall, libheimdallcore],
        link_args: gc_sections_linkargs,
        include_directories: [heimdall_include_dirs],
        dependencies: heimdall_deps,
        c_args: [heimdall_top_builddir_def, heimdall_top_srcdir_def],
        objc_args: [heimdall_top_builddir_def, heimdall_top_srcdir_def],
        win_subsystem: 'windows'
    )

    heimdall_cache_gen_dep = executable('heimdall-cache-gen',
        ['cachegen.c'] + heimdall_win_extra,
        link_with: [heimdall_libcompat, libheimdall],
        link_args: gc_sections_linkargs,
        include_directories: [heimdall_include_dirs],
        install: true,
        install_dir: pkglibexecdir_path,
    )

    if not meson.is_cross_build() or find_program('exe_wrapper', required: false).found()
    plugins_dat  = custom_target('plugins.dat',
        # `heimdall_cache_gen_dep.full_path()` doesn't work since the executable is not built yet.
        # `heimdall_cache_gen_dep` creates a build order dependency.
        command: [heimdall_cache_gen_dep, meson.global_build_root() / 'modules'],
        # ninja command will be `ninja bin/plugins.dat`
        output: 'plugins.dat',
        depends: heimdall_plugins_targets_array,
        build_by_default: not meson.is_cross_build(),
    )
    endif

    heimdall_preparser_sources = [
      'preparser/main.c',
      'preparser/cmdline.c',
      'preparser/cmdline.h',
      '../src/config/getopt.c',
      '../src/config/jaro_winkler.c',
    ]

    executable('heimdall-preparser',
        heimdall_preparser_sources + heimdall_win_extra,
        link_with: [libheimdall, libheimdallcore, heimdall_libcompat],
        link_args: gc_sections_linkargs,
        include_directories: [heimdall_include_dirs],
        dependencies: heimdall_deps,
        install: true,
        install_dir: pkglibexecdir_path,
        win_subsystem: 'windows'
    )

    heimdall_preparser_static_dep = executable('heimdall-preparser-static',
        heimdall_preparser_sources + heimdall_win_extra,
        link_with: [libheimdall, libheimdallcore, heimdall_libcompat],
        link_args: gc_sections_linkargs,
        include_directories: [heimdall_include_dirs],
        dependencies: heimdall_deps,
        c_args: [heimdall_top_builddir_def, heimdall_top_srcdir_def],
        objc_args: [heimdall_top_builddir_def, heimdall_top_srcdir_def],
        install_dir: pkglibexecdir_path,
        win_subsystem: 'windows'
    )

endif
