Instructions to code your own Heimdall Lua playlist script.

See lua/README.txt for generic documentation about Lua usage in Heimdall.

Examples: See liveleak.lua, dailymotion.lua,
          soundcloud.lua, cue.lua ...

Heimdall Lua playlist modules should define two functions:
 * probe(): returns true if we want to handle the playlist in this script
 * parse(): read the incoming data and return playlist item(s)
            Playlist items use the same format as that expected in the
            playlist.add() function (see general lua/README.txt)

Heimdall defines a global heimdall object with the following members:
 * heimdall.path: the URL string (without the leading http:// or file:// element)
 * heimdall.access: the access used ("http" for http://, "file" for file://, etc.)
 * heimdall.peek( <int> ): return the first <int> characters from the playlist file.
 * heimdall.read( <int> ): read <int> characters from the playlist file.
                      THIS FUNCTION CANNOT BE USED IN probe().
 * heimdall.readline(): return a new line of playlist data on each call.
                   THIS FUNCTION CANNOT BE USED IN probe().

Available Heimdall specific Lua modules: msg, strings, stream, variables and
xml. See lua/README.txt.
