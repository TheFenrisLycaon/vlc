--[[ This code is public domain (since it really isn't very interesting) ]]--

msg = [[
This is the `dummy' Heimdall Lua interface module.
Please specify a Heimdall Lua interface to load with the --lua-intf option.
Heimdall Lua interface modules include: `cli' and `http'.
For example: heimdall -I luaintf --lua-intf cli
You can also use the alternate syntax: heimdall -I "luaintf{intf=cli}"
See share/lua/intf/README.txt for more information about lua interface modules.]]

for line in string.gmatch(msg,"([^\n]+)\n*") do
    heimdall.msg.err(line)
end

heimdall.misc.quit()
