local UtilSys = require("src/engine/util")


local headlessClientMetatable = {}
function headlessClientMetatable.__index()
	return UtilSys.noop
end

local headlessclient = setmetatable({}, headlessClientMetatable)
function headlessclient.writeData(filename, dataStr)
	return UtilSys.writeDataUncompressed(filename, dataStr)
end
function headlessclient.readData(filename)
	return UtilSys.readDataUncompressed(filename)
end


-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local client = jeClientBindings or headlessclient  -- luacheck: globals jeClientBindings
client.state = {
	["running"] = false,
	["fps"] = 0,
	["inputLeft"] = false,
	["inputRight"] = false,
	["inputUp"] = false,
	["inputDown"] = false,
	["inputA"] = false,
	["inputB"] = false,
	["inputX"] = false,
	["inputY"] = false,
}
function client.runTests()
	client.writeData("clientTestFile", "")
	assert(client.readData("clientTestFile") == "")
	os.remove("clientTestFile")

	client.step(client.state)
end

return client
