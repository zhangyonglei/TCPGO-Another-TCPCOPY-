local m = {}

function m.des()
	return "hit " .. m.count .. " times. Yet another test lua script."
end

m.count = 0

function m.main(client_ip, req, resp)
	if req:match("LD=") then
		m.count = m.count + 1
	end
	save_traffic("/tmp/hello.pcap")
	print("Running test case record.")
end

return m;
