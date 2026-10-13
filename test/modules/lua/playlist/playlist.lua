local check_peek_read = "<html>"

function probe()
	heimdall.msg.dbg("Access:" .. heimdall.access)
	heimdall.msg.dbg("Path:" .. heimdall.path)
	local peeked = heimdall.peek(6)
	return heimdall.access == "mock" and heimdall.path == "length=100" and peeked == check_peek_read
end

function parse()
	heimdall.msg.dbg("parse is triggered ")
	local read_bytes = heimdall.read(6)
	if read_bytes == check_peek_read then
		heimdall.msg.dbg("read is triggered")
	end
	local line = heimdall.readline()
	local title = string.match(line, "<title>(.-)</title>")
	if title ~= nil then
		heimdall.msg.dbg("readline is triggered")
	end
	return { { name = title, path = heimdall.access .. "://" .. heimdall.path } }
end
