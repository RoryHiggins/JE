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
function HeadlessClientSys.isRunning()
	return false
end
function HeadlessClientSys.getCurrentFPS()
	return 0
end
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
function ClientSys.runTests()
	ClientSys.isRunning()
	ClientSys.step()
end

return ClientSys
