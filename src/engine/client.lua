local util = require("src/engine/util")

local headlessClientMetatable = {}
function headlessClientMetatable.__index()
	return util.noop
end

local headlessclient = setmetatable({}, headlessClientMetatable)
function headlessclient.writeData(filename, dataStr)
	return util.writeDataUncompressed(filename, dataStr)
end
function headlessclient.readData(filename)
	return util.readDataUncompressed(filename)
end

-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local client = jeClientBindings or headlessclient  -- luacheck: globals jeClientBindings
client.state = {
	["running"] = false,
	["fps"] = 0,
	["logLevel"] = util.logLevel,
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
