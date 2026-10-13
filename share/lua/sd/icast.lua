--[[

 Copyright © 2010 VideoLAN and AUTHORS
 Copyright (C) 2026 TheFenrisLycaon

 Authors: Julien 'Lta' BALLET <contact at lta dot io>

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
--]]
local config = {}
      config["base_url"]     = "http://api.icast.io"
      config["api_base_url"] = config.base_url.."/1"

local json   = nil
local roots  = nil

function json_init()
    if json ~= nil then return false end

    heimdall.msg.dbg("JSON parser lazy initialization")
    json = require ("dkjson")

    -- Use heimdall.stream to grab a remote json file, place it in a string,
    -- decode it and return the decoded data.
    json["parse_url"] = function(url)
        local stream = heimdall.stream(url)
        local string = ""
        local line   = ""

        repeat
            line = stream:readline()
            string = string..line
        until line ~= nil

        --print(string)
        return json.decode(string)
    end
end

-- Heimdall SD mandatory method, return the name and capabilities of this SD.
function descriptor()
    return { title = "iCast Stream Directory", capabilities = {"search"} }
end

-- Utility function to replace nils by an empty string, borrowed from icecast.lua
function dropnil(s)
    if s == nil then return "" else return s end
end

function roots_init()
    if roots ~= nil then return nil end

    roots = {
        locals  = heimdall.sd.add_node({title="Local Streams"}),
        cats    = heimdall.sd.add_node({title="Categories"}),
        search  = nil
    }
end

-- Add this station to the discovered service list.
function station_add(node, station)
    if station.streams == nil or station.streams[1] == nil then
        return nil
    end

    item = node:add_subitem({
        title = station.name,
        path  = station.streams[1]["uri"],
        meta  = {
            ["Listing Source"]  = "icast.io",
            ["Listing Type"]    = "radio",
            ["Icecast Bitrate"] = dropnil(station.streams[1].bitrate)
        }
    })

    if station.slogan then
        item:set_name(station.name..": "..station.slogan)
    end
    if station.genre_list then
        item:set_genre(table.concat(station.genre_list, ", "))
    end
    if station.language then
        item:set_language(station.language)
    end
    if station.logo.medium then
        item:set_arturl(config.base_url..station.logo.medium)
    end
end

-- Get an url to iCast's API, parse the returned stations and add them
-- to the list of discovered medias.
function stations_fetch(node, url)
    json_init()
    heimdall.msg.info("Fetching stations from API ("..url..")")

    local result = json.parse_url(config.api_base_url..url)

    if result.stations == nil then
        return nil
    end

    for _, station in ipairs(result.stations) do
        station_add(node, station)
    end
end

-- Heimdall SD API - Search entry point
function search(query)
    if roots.search then heimdall.sd.remove_node(roots.search) end

    roots.search = heimdall.sd.add_node({title="Search Results"})

    stations_fetch(roots.search, "/stations/search.json?q="..query.."*")
end

-- Heimdall SD API - Main listing entry point
function main()
    roots_init()

    stations_fetch(roots.locals, "/stations/local.json")

    heimdall.msg.dbg("Fetching list of genre from API (/genres.json)")
    local result = json.parse_url(config.api_base_url.."/genres.json")
    for genre, sub_genres in pairs(result.genres) do
        local node = roots.cats:add_subnode({title=genre})
        for _, sub_genre in ipairs(sub_genres) do
            local sub_node = node:add_subnode({title=sub_genre})
            stations_fetch(sub_node, "/stations/genre/"..sub_genre..".json")
        end
    end
end
