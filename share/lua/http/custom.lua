-- make xgettext fetch strings from html code
function gettext(text) print(heimdall.gettext._(text)) end

local _G = _G

local dialogs = setmetatable({}, {
__index = function(self, name)
    -- Cache the dialogs
    return rawget(self, name) or
           rawget(rawset(self, name, process(http_dir.."/dialogs/"..name)), name)
end})

_G.dialogs = function(...)
    for i=1, select("#",...) do
        dialogs[(select(i,...))]()
    end
end

_G.vlm = heimdall.vlm()

local _rd = nil

_G.get_renderer_discovery = function()
    if not _rd then
        _rd = heimdall.rd.create("mdns_renderer")
    end
    return _rd
end

