local util = require("src/engine/util")

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
	["logLevel"] = util.logLevel,
	["testsEnabled"] = true,
	["testsLogLevel"] = util.LOG_LEVEL_WARN,
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
function headlessClient.onRunTests()
	headlessClient.writeData("clientTestFile", "")
	assert(headlessClient.readData("clientTestFile") == "")
	os.remove("clientTestFile")
end

-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local client = jeClientBindings or headlessClient  -- luacheck: globals jeClientBindings

return client
