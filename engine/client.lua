local log = require("engine/util/log")
local util = require("engine/util/util")

local headlessClientMetatable = {}
function headlessClientMetatable.__index()
	return util.noop
end

local headlessClient = setmetatable({}, headlessClientMetatable)
headlessClient.state = {
	["running"] = false,
	["width"] = 0,
	["height"] = 0,
	["fps"] = 0,
	["logLevel"] = log.logLevel,
	["testsEnabled"] = true,
	["testsLogLevel"] = log.testsLogLevel,
	["inputLeft"] = false,
	["inputRight"] = false,
	["inputUp"] = false,
	["inputDown"] = false,
	["inputA"] = false,
	["inputB"] = false,
	["inputX"] = false,
	["inputY"] = false,
}
function headlessClient.writeData(filename, dataStr)
	return util.writeDataUncompressed(filename, dataStr)
end
function headlessClient.readData(filename)
	return util.readDataUncompressed(filename)
end

-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local client = jeLuaClientBindings or headlessClient  -- luacheck: globals jeLuaClientBindings
client.SYSTEM_NAME = "client"
function client.onRunTests()
	headlessClient.writeData("clientTestFile", "")
	assert(headlessClient.readData("clientTestFile") == "")
	os.remove("clientTestFile")

	local numTestSuites = 1
	if client ~= headlessClient then
		numTestSuites = client.runTests()
	end
	return numTestSuites
end

return client
