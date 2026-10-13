heimdall_tests += {
    'name' : 'test_libheimdall_core',
    'sources' : files('core.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall],
    'module_depends' : heimdall_plugins_targets.keys()
}

heimdall_tests += {
    'name' : 'test_libheimdall_equalizer',
    'sources' : files('equalizer.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall]
}

heimdall_tests += {
    'name' : 'test_libheimdall_media',
    'sources' : files('media.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends': heimdall_plugins_targets.keys()
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_callback',
    'sources' : files('media_callback.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends': ['imem'],
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_thumbnail_argb',
    'sources' : files('thumbnailer.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'suite' : ['libheimdall', 'test_libheimdall_media_thumbnail'],
    'c_args' : [
        '-DTEST_THUMB_TYPE=libheimdall_picture_Argb',
        '-DTEST_REQUIRED_MODULES="avcodec"',
    ],
    'module_depends': ['demux_mock', 'swscale', 'avcodec'],
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_thumbnail_rgba',
    'sources' : files('thumbnailer.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'suite' : ['libheimdall', 'test_libheimdall_media_thumbnail'],
    'c_args' : [
        '-DTEST_THUMB_TYPE=libheimdall_picture_Rgba',
        '-DTEST_REQUIRED_MODULES="avcodec"',
    ],
    'module_depends': ['demux_mock', 'swscale', 'avcodec'],
}


heimdall_tests += {
    'name' : 'test_libheimdall_media_thumbnail_jpeg',
    'sources' : files('thumbnailer.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'suite' : ['libheimdall', 'test_libheimdall_media_thumbnail'],
    'c_args' : [
        '-DTEST_THUMB_TYPE=libheimdall_picture_Jpg',
        '-DTEST_REQUIRED_MODULES="jpeg"',
    ],
    'module_depends': ['demux_mock', 'swscale', 'jpeg'],
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_thumbnail_png',
    'sources' : files('thumbnailer.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'suite' : ['libheimdall', 'test_libheimdall_media_thumbnail'],
    'c_args' : [
        '-DTEST_THUMB_TYPE=libheimdall_picture_Png',
        '-DTEST_REQUIRED_MODULES="png"',
    ],
    'module_depends': ['demux_mock', 'swscale', 'png'],
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_thumbnail_webp',
    'sources' : files('thumbnailer.c'),
    'link_with' : [libheimdall, libheimdallcore],
    'suite' : ['libheimdall', 'test_libheimdall_media_thumbnail'],
    'c_args' : [
        '-DTEST_THUMB_TYPE=libheimdall_picture_WebP',
        '-DTEST_REQUIRED_MODULES="vpx"',
    ],
    'module_depends': ['demux_mock', 'swscale', 'vpx'],
}

# heimdall_tests += {
#     'name' : 'test_libheimdall_media_list_player',
#     'sources' : files('libheimdall/media_list_player.c'),
#     'link_with' : [libheimdall]
# }

heimdall_tests += {
    'name' : 'test_libheimdall_media_list',
    'sources' : files('media_list.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall]
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_player',
    'sources' : files('media_player.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends': ['demux_mock']
}

heimdall_tests += {
    'name' : 'test_libheimdall_video_callback',
    'sources' : files('video_callback.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall, libheimdallcore],
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_player_record',
    'sources' : files('media_player_record.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends': ['demux_mock', 'stream_out_record']
}

heimdall_tests += {
    'name' : 'test_libheimdall_media_discoverer',
    'sources' : files('media_discoverer.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall]
}

heimdall_tests += {
    'name' : 'test_libheimdall_renderer_discoverer',
    'sources' : files('renderer_discoverer.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall]
}

heimdall_tests += {
    'name' : 'test_libheimdall_slaves',
    'sources' : files('slaves.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends' : heimdall_plugins_targets.keys()
}

heimdall_tests += {
    'name' : 'test_libheimdall_meta',
    'sources' : files('meta.c'),
    'suite' : ['libheimdall'],
    'link_with' : [libheimdall, libheimdallcore],
    'module_depends' : heimdall_plugins_targets.keys()
}
