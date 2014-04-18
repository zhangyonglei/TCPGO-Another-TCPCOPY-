local m = {}

m.count = 0
m.saved = 0

function m.main(client_ip, client_port, req, resp)
	if req:match("ad_type=TP&l=4002") then
		m.count = m.count + 1
		if m.saved < 10 and resp:len() > 400 then
			local s = string.format("/tmp/4002.%d.pcap", m.count)
			save_traffic(s)
			m.saved = m.saved + 1
		end
	end
end

function m.desp()
	return "hit " .. m.count .. " times."
end

return m


