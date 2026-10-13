heimdall_modules += {
    'name' : 'cache_read',
    'sources' : files('cache_read.c'),
    'plugin_dir' : 'stream_filter',
}

if host_system != 'windows' and not have_tvos
  heimdall_modules += {
      'name' : 'decomp',
      'sources' : files('decomp.c'),
      'dependencies' : [threads_dep],
      'plugin_dir' : 'stream_filter',
  }
endif

if z_dep.found()
  heimdall_modules += {
      'name' : 'inflate',
      'sources' : files('inflate.c'),
      'dependencies' : [z_dep],
      'plugin_dir' : 'stream_filter',
  }
endif

heimdall_modules += {
    'name' : 'prefetch',
    'sources' : files('prefetch.c'),
    'plugin_dir' : 'stream_filter',
    'enabled' : not have_win_store
}

heimdall_modules += {
    'name' : 'hds',
    'sources' : files('hds/hds.c'),
    'plugin_dir' : 'stream_filter',
}

heimdall_modules += {
    'name' : 'record',
    'sources' : files('record.c'),
    'plugin_dir' : 'stream_filter',
}

heimdall_modules += {
    'name' : 'adf',
    'sources' : files('adf.c'),
    'plugin_dir' : 'stream_filter',
}

heimdall_modules += {
    'name' : 'skiptags',
    'sources' : files('skiptags.c'),
    'plugin_dir' : 'stream_filter',
}

aribb25_dep = dependency('aribb25', version:'>= 0.2.6', required: get_option('aribb25'))
heimdall_modules += {
    'name' : 'aribcam',
    'sources' : files('aribcam.c'),
    'dependencies' : [aribb25_dep],
    'plugin_dir' : 'stream_filter',
    'enabled': aribb25_dep.found(),
}
