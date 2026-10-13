heimdall_libcompat = []

if libcompat_sources.length() > 0
    heimdall_libcompat = static_library(
        'compat',
        libcompat_sources,
        include_directories: [heimdall_include_dirs],
        dependencies: [m_lib],
        pic: true,
        install: host_system != 'windows',
        install_dir: get_option('libdir') / 'heimdall'
    )
endif

heap_alloc_env = environment({'ASAN_OPTIONS': 'allocator_may_return_null=1'})
heimdall_tests += {
    'name': 'heap',
    'sources': files('test/heap.c'),
    'suite': ['compat'],
    'link_with': [heimdall_libcompat],
    'env': heap_alloc_env,
}

heimdall_tests += {
    'name': 'strnstr',
    'sources': files('test/strnstr.c'),
    'suite': ['compat'],
    'link_with': [heimdall_libcompat],
}

heimdall_tests += {
    'name': 'ckd_ckd',
    'sources': files('test/ckd.c'),
    'c_args': ['-DTEST_CKD'],
    'suite': ['compat'],
    'link_with': [heimdall_libcompat],
}

ckd_builtin_cargs = ['-DTEST_BUILTIN']
if cdata.has('HAVE_STDCKDINT_H')
    ckd_builtin_cargs += '-DHAVE_CKD'
endif
heimdall_tests += {
    'name': 'ckd_builtin',
    'sources': files('test/ckd.c'),
    'c_args': ckd_builtin_cargs,
    'suite': ['compat'],
    'link_with': [heimdall_libcompat],
}

heimdall_tests += {
    'name': 'ckd_compat',
    'sources': files('test/ckd.c'),
    'c_args': ['-DTEST_COMPAT'],
    'suite': ['compat'],
    'link_with': [heimdall_libcompat],
}

#TODO: add clock_nanosleep test for Darwin
