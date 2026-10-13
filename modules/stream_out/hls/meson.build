# HLS Stream output modules

heimdall_modules += {
    'name' : 'stream_out_hls',
    'sources' : files(
        'hls.c',
        'variant_maps.c',
        'storage.c',
        'segments.c',
        'codecs.c',
        'subtitles_segmenter.c'
    ),
    'link_with' : [hxxxhelper_lib],
    'shortname' : 's_o_hls',
    'plugin_dir' : 'stream_out',
}
