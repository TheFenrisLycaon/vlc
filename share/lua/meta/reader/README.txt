Instructions to code your own Heimdall Lua meta script.

See lua/README.txt for generic documentation about Lua usage in Heimdall.

Examples: See filename.lua .

Heimdall Lua "meta reader" modules should define one of the following functions:
 * read_meta(): returns a path to an artwork for the given item

Available Heimdall specific Lua modules: msg, stream, strings, variables, item,
objects and xml. See lua/README.txt

Note, those scripts are supposed to be fast. Read non blocking, no IO.
