--[==========================================================================[
dumpmeta.lua: dump a file's meta data on stdout/stderr
--[==========================================================================[
 Copyright (C) 2010 the VideoLAN team
 Copyright (C) 2026 TheFenrisLycaon

 Authors: Antoine Cellerier <dionoea at lycaria dot org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
--]==========================================================================]

--[[ to dump meta data information in the debug output, run:
       heimdall -I luaintf --lua-intf dumpmeta coolmusic.mp3
     Additional options can improve performance and output readability:
       -V dummy -A dummy --no-video-title --no-media-library -q
--]]

local item
repeat
    item = heimdall.player.item()
until (item and item:is_preparsed())

-- preparsing doesn't always provide all the information we want (like duration)
repeat
until item:stats()["demux_read_bytes"] > 0

heimdall.msg.info("name: "..item:name())
heimdall.msg.info("uri: "..heimdall.strings.decode_uri(item:uri()))
heimdall.msg.info("duration: "..tostring(item:duration()))

heimdall.msg.info("meta data:")
local meta = item:metas()
if meta then
    for key, value in pairs(meta) do
        heimdall.msg.info("  "..key..": "..value)
    end
else
    heimdall.msg.info("  no meta data available")
end

heimdall.msg.info("info:")
for cat, data in pairs(item:info()) do
    heimdall.msg.info("  "..cat)
    for key, value in pairs(data) do
        heimdall.msg.info("    "..key..": "..value)
    end
end

heimdall.misc.quit()
