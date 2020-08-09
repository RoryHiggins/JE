local UtilSys = require("src/engine/util")

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
	return HeadlessClientSys.writeDataRaw(filename, dataStr)
end
function HeadlessClientSys.readData(filename)
	return HeadlessClientSys.readDataRaw(filename)
end

-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
local ClientSys = jeClientBindings or HeadlessClientSys  -- luacheck: globals jeClientBindings
ClientSys.width = 160  -- TODO find a better way to store/accessing this
ClientSys.height = 120
function ClientSys.writeDataRaw(filename, dataStr)
	local file, errMsg = io.open(filename, "w")
	if file == nil then
		UtilSys.err("ClientSys.writeDataRaw(): io.open() failed, filename=%s, error=%s", filename, errMsg)
		return false
	end

	file:write(dataStr)
	file:close()

	return true
end
function ClientSys.readDataRaw(filename)
	local file, errMsg = io.open(filename, "r")
	if file == nil then
		UtilSys.err("ClientSys.readDataRaw(): io.open() failed, filename=%s, error=%s", filename, errMsg)
		return
	end

	local dataStr = file:read("*all")
	file:close()

	return dataStr
end
function ClientSys.runTests()
	ClientSys.isRunning()
	ClientSys.step()
end

return ClientSys
