local UtilSys = require("src/engine/util")

local function writeDataUncompressed(filename, dataStr)
	local file, errMsg = io.open(filename, "w")
	if file == nil then
		UtilSys.err("ClientSys.writeDataUncompressed(): io.open() failed, filename=%s, error=%s", filename, errMsg)
		return false
	end

	file:write(dataStr)
	file:close()

	return true
end
local function readDataUncompressed(filename)
	local file, errMsg = io.open(filename, "r")
	if file == nil then
		UtilSys.err("ClientSys.readDataUncompressed(): io.open() failed, filename=%s, error=%s", filename, errMsg)
		return
	end

	local dataStr = file:read("*all")
	file:close()

	return dataStr
end


local HeadlessClientMetatable = {}
function HeadlessClientMetatable.__index()
	return UtilSys.noop
end

local HeadlessClientSys = setmetatable({}, HeadlessClientMetatable)
function HeadlessClientSys.writeData(filename, dataStr)
	return writeDataUncompressed(filename, dataStr)
end
function HeadlessClientSys.readData(filename)
	return readDataUncompressed(filename)
end


-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local ClientSys = jeClientBindings or HeadlessClientSys  -- luacheck: globals jeClientBindings
ClientSys.writeDataUncompressed = writeDataUncompressed
ClientSys.readDataUncompressed = readDataUncompressed
ClientSys.state = {
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
function ClientSys.runTests()
	ClientSys.writeData("ClientSysTestFile", "")
	assert(ClientSys.readData("ClientSysTestFile") == "")
	os.remove("ClientSysTestFile")

	ClientSys.step(ClientSys.state)
end

return ClientSys
