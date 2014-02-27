local m = {}

function m.request(req)
	print(req)
end

function m.response(resp)
	print(resp)
end

function m.main(client_ip, req, resp)
	print(client_ip)
	m.request(req)	
	m.response(resp)
end

return m;
