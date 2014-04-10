local m = {}

function m.main(client_ip, port, req, resp)
	local r = math.random(1, 1000)
	if r > 10 then
		return
	end

	local req_file_name = client_ip .. "_" .. port .. ".req"
	local resp_file_name = client_ip .. "_" .. port .. ".resp"

	local req_file_handle = io.open(req_file_name .. ".tmp", "w+")	
	local resp_file_handle = io.open(resp_file_name .. ".tmp", "w+")

	req_file_handle:write(req)
	resp_file_handle:write(resp)

	req_file_handle:close()
	resp_file_handle:close()

	os.rename(req_file_name .. ".tmp", req_file_name)
	os.rename(resp_file_name .. ".tmp", resp_file_name)
end

return m
