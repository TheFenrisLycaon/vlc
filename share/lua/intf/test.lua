function assert_url(result, protocol, username, password, host, port, path)
    assert(result.protocol == protocol)
    assert(result.username == username)
    assert(result.password == password)
    assert(result.host     == host)
    assert(result.port     == port)
    assert(result.path     == path)
end

heimdall.msg.info('---- Testing misc functions ----')
heimdall.msg.info('version: ' .. heimdall.misc.version())
heimdall.msg.info('copyright: ' .. heimdall.misc.copyright())
heimdall.msg.info('license: ' .. heimdall.misc.license())
heimdall.msg.info('mdate: ' .. heimdall.misc.mdate())

heimdall.msg.info('---- Testing config functions ----')
heimdall.msg.info('datadir: ' .. heimdall.config.datadir())
heimdall.msg.info('userdatadir: ' .. heimdall.config.userdatadir())
heimdall.msg.info('homedir: ' .. heimdall.config.homedir())
heimdall.msg.info('configdir: ' .. heimdall.config.configdir())
heimdall.msg.info('cachedir: ' .. heimdall.config.cachedir())

heimdall.msg.info('---- Testing net functions ----')
heimdall.msg.info(' * testing heimdall.strings.url_parse')
heimdall.msg.info('    "filename.ext"')
assert_url(heimdall.strings.url_parse('file:///filename.ext'), 'file', nil, nil,
           nil, 0, '/filename.ext')
heimdall.msg.info('    "http://server.org/path/file.ext"')
assert_url(heimdall.strings.url_parse('http://server.org/path/file.ext'),
           'http', nil, nil, 'server.org', 0, '/path/file.ext')
heimdall.msg.info('    "rtmp://server.org:4212/bla.ext"')
assert_url(heimdall.strings.url_parse('rtmp://server.org:4212/bla.ext'),
           'rtmp', nil, nil, 'server.org', 4212, '/bla.ext')
heimdall.msg.info('    "ftp://userbla@server.org:4567/bla.ext"')
assert_url(heimdall.strings.url_parse('rtmp://userbla@server.org:4567/bla.ext'),
           'rtmp', 'userbla', nil, 'server.org', 4567, '/bla.ext')
heimdall.msg.info('    "sftp://userbla:Passw0rd@server.org/bla.ext"')
assert_url(heimdall.strings.url_parse('sftp://userbla:Passw0rd@server.org/bla.ext'),
           'sftp', 'userbla', 'Passw0rd', 'server.org', 0, '/bla.ext')
heimdall.msg.info("")

heimdall.misc.quit()
