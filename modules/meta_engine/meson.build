# Folder
heimdall_modules += {
    'name' : 'folder',
    'sources' : files('folder.c'),
    'plugin_dir' : 'meta_engine',
}

# Taglib
taglib_dep = dependency('taglib', version: '>= 1.11', required: get_option('taglib'))
if taglib_dep.found()
    heimdall_modules += {
        'name' : 'taglib',
        'sources' : files('taglib.cpp'),
        'dependencies' : [taglib_dep, z_dep],
        'plugin_dir' : 'meta_engine',
    }
endif
