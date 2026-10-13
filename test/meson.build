subdir('src')
subdir('modules')
subdir('libheimdall')

foreach heimdall_test: heimdall_tests
    if not heimdall_test.has_key('name')
        error('Got invalid heimdall_tests entry without \'name\' key')
    endif
    if not heimdall_test.has_key('sources')
        error('Got invalid heimdall_tests entry without \'sources\' key')
    endif

    # This list MUST be kept in sync with the keys used below!
    valid_dict_keys = [
        'name',
        'sources',
        'moc_headers',
        'moc_sources',
        'suite',
        'link_with',
        'link_args',
        'module_depends',
        'dependencies',
        'c_args',
        'cpp_args',
        'objc_args',
        'include_directories',
        'env',
        'enabled',
        'wrapper',
    ]

    foreach key : heimdall_test.keys()
        if key not in valid_dict_keys
            error('Invalid key \'@0@\' found in heimdall_tests entry for \'@1@\''
                .format(key, heimdall_test['name']))
        endif
    endforeach

    # Skip disabled tests
    if not heimdall_test.get('enabled', true)
        continue
    endif

    common_args = [
        '-DSRCDIR="@0@"'.format(heimdall_src_root + '/test/'),
        '-DTOP_BUILDDIR="@0@"'.format(heimdall_build_root),
        '-DTOP_SRCDIR="@0@"'.format(heimdall_src_root),
    ]

    disabled_dependencies = []
    test_modules_deps = []
    foreach module_name : heimdall_test.get('module_depends', [])
        if module_name not in heimdall_plugins_manifest.keys()
            error('Test entry \'@0@\' cannot depend on non-existant Heimdall plugin \'@1@\''
                  .format(heimdall_test['name'], module_name))
        endif

        if not heimdall_plugins_manifest[module_name].get('enabled', true)
            disabled_dependencies += module_name
        else
            test_modules_deps += heimdall_plugins_targets[module_name]
        endif
    endforeach

    if disabled_dependencies != []
        # TODO: mark as skipped
        warning('Test \'@0@\' will be disabled, missing plugins: @1@'.format(
                heimdall_test['name'], ', '.join(disabled_dependencies)))
        continue
    endif

    moc_sources = []
    if (heimdall_test.has_key('moc_headers') or heimdall_test.has_key('moc_sources')) and qt6_dep.found()
        moc_sources += qt6.preprocess(
            moc_headers: heimdall_test.get('moc_headers', []),
            moc_sources: heimdall_test.get('moc_sources', []),
            include_directories: qt_include_dir,
            dependencies: qt6_dep)
    endif

    test_exe = executable(heimdall_test['name'], heimdall_test['sources'], moc_sources,
        build_by_default: false,
        link_with: [heimdall_test.get('link_with', []),
            heimdall_libcompat],
        link_args: [heimdall_test.get('link_args', [])],
        include_directories: [heimdall_test.get('include_directories', []),
            heimdall_include_dirs],
        dependencies: [heimdall_test.get('dependencies', []),
            libheimdallcore_deps, opengl_dep],
        c_args: [heimdall_test.get('c_args', []), common_args],
        cpp_args: [heimdall_test.get('cpp_args', []), common_args],
        objc_args: [heimdall_test.get('objc_args', []), common_args])

    # Handle optional test wrapper (e.g., xvfb-run for X11 tests)
    # The wrapper must be a program object
    if heimdall_test.has_key('wrapper')
        test_wrapper = heimdall_test['wrapper']
        if test_wrapper.found()
            test(heimdall_test['name'],
                test_wrapper,
                args: [test_exe],
                env: heimdall_test.get('env', []),
                suite: [heimdall_test.get('suite', []), 'test'],
                depends: [test_modules_deps])
        else
            warning('Test \'@0@\' skipped: wrapper not found'.format(
                heimdall_test['name']))
        endif
    else
        test(heimdall_test['name'],
            test_exe,
            env: heimdall_test.get('env', []),
            suite: [heimdall_test.get('suite', []), 'test'],
            depends: [test_modules_deps])
    endif
endforeach

libheimdall_demux_defines = []
libheimdall_demux_deps = []
# TODO support !HAVE_DYNAMIC_PLUGINS
# libheimdall_demux_defines=['-DHAVE_STATIC_MODULES']
# if libdvbpsi_dep.found()
#     libheimdall_demux_defines += '-DHAVE_DVBPSI'
#     libheimdall_demux_deps += [libdvbpsi_dep, aribb24_dep, libdvbcsa_dep]
# endif
# if libebml_dep.found() and libmatroska_dep.found()
#     libheimdall_demux_defines += '-DHAVE_MATROSKA'
#     libheimdall_demux_deps += [libebml_dep, libmatroska_dep, z_dep]
# endif

libheimdall_demux_run = static_library ('libheimdall_demux_run',
    files('src/input/demux-run.c', 'src/input/common.c', '../src/input/var.c'),
    c_args: libheimdall_demux_defines,
    dependencies: libheimdall_demux_deps,
    include_directories: [heimdall_include_dirs],
    link_with: [libheimdall, libheimdallcore, heimdall_libcompat])

libheimdall_demux_dec_defines = [libheimdall_demux_defines, '-DHAVE_DECODERS']
libheimdall_demux_dec_run = static_library ('libheimdall_demux_dec_run',
    files('src/input/demux-run.c', 'src/input/common.c', '../src/input/var.c', 'src/input/decoder.c'),
    c_args: libheimdall_demux_dec_defines,
    dependencies: libheimdall_demux_deps,
    include_directories: [heimdall_include_dirs],
    link_with: [libheimdall, libheimdallcore, heimdall_libcompat])

executable('heimdall-demux-run', 'heimdall-demux-run.c',
    include_directories: [heimdall_include_dirs],
    link_with: [libheimdall_demux_run, libheimdall, libheimdallcore, heimdall_libcompat],
    install: false,
    win_subsystem: 'console')

executable('heimdall-demux-dec-run', 'heimdall-demux-run.c',
    include_directories: [heimdall_include_dirs],
    link_with: [libheimdall_demux_dec_run, libheimdall, libheimdallcore, heimdall_libcompat],
    install: false,
    win_subsystem: 'console')

executable('heimdall-window', 'heimdall-window.c',
    include_directories: [heimdall_include_dirs],
    link_with: [libheimdall, libheimdallcore, heimdall_libcompat],
    c_args: common_args,
    install: false,
    win_subsystem: 'console')
