local json = require("lib/json")
local util = require("src/engine/util")
local client = require("src/engine/client")


local Simulation = {}
function Simulation:broadcast(event, ...)
	for _, system in pairs(self.systems) do
		local eventHandler = system[event]
		if eventHandler then
			eventHandler(system, ...)
		end
	end
end
function Simulation:addSystem(class)
	if type(class) ~= "table" then
		util.err("Simulation:addSystem() class is not a table, class=%s", util.toComparable(class))
		return {}
	end

	local systemName = class.SYSTEM_NAME

	if type(systemName) ~= "string" then
		util.err("Simulation:addSystem() class.SYSTEM_NAME is not valid, class=%s", util.toComparable(class))
		return {}
	end

	local system = self.systems[systemName]
	if system == nil then
		util.debug("Simulation:addSystem() instantiating system, systemName=%s", systemName)

		class.__index = class
		system = setmetatable({}, class)

		system.simulation = self
		system.created = false

		self.systems[systemName] = system
	end

	if self.created and system.onSimulationCreate and not system.created then
		util.debug("Simulation:addSystem() creating system, systemName=%s", systemName)

		system.created = true  -- marking as created first, to prevent recursion with circular dependencies
		system:onSimulationCreate()
	end

	return system
end
function Simulation:getInputs()
	local previousInputs = self.inputs or {}
	local clientInputMap = {
		["left"] = "inputLeft",
		["right"] = "inputRight",
		["up"] = "inputUp",
		["down"] = "inputDown",
		["a"] = "inputA",
		["b"] = "inputB",
		["x"] = "inputX",
		["y"] = "inputY",
	}

	local inputs = {}

	for inputKey, clientInputKey in pairs(clientInputMap) do
		local input = {}
		input.down = client.state[clientInputKey]

		input.pressed = false
		input.released = false
		input.framesDown = 0
		if previousInputs[inputKey] then
			input.pressed = input.down and not previousInputs[inputKey].down
			input.released = not input.down and previousInputs[inputKey].down
			if input.down then
				input.framesDown = previousInputs[inputKey].framesDown + 1
			end
		end

		inputs[inputKey] = input
	end

	self.inputs = inputs
end
function Simulation.isRunning()
	return client.state.running
end
function Simulation:step()
	client.step()
	self:getInputs()
	self:broadcast("onSimulationStep")
	self:broadcast("onSimulationDraw")
end
function Simulation:destroy()
	if not self.created then
		return
	end

	util.log("Simulation:destroy()")

	self:broadcast("onSimulationDestroy")
	self.state = {}

	for _, system in pairs(self.systems) do
		system.created = false
	end

	self.created = false
end
function Simulation:create()
	self:destroy()

	util.log("Simulation:create()")

	math.randomseed(0)

	self.created = true
	self.state.saveVersion = 1

	self:getInputs()

	for systemName, system in pairs(self.systems) do
		if system.simulation and not system.created then
			util.debug("Simulation:create() creating system, name=%s", systemName)

			system.created = true  -- marking as created first, to prevent recursion with circular dependencies

			if system.onSimulationCreate then
				system:onSimulationCreate()
			end
		end
	end
end
function Simulation:dump(filename)
	util.debug("Simulation:dump(): filename=%s", filename)

	local dump = {
		["state"] = self.state,
		["static"] = self.static,
		["systems"] = util.getKeys(self.systems),
	}
	if not util.writeDataUncompressed(filename, util.toComparable(dump)) then
		util.err("Simulation:dump(): client.writeData() failed")
		return false
	end

	return true
end
function Simulation:save(filename)
	util.debug("Simulation:save(): filename=%s", filename)

	if not client.writeData(filename, json.encode(self.state)) then
		util.err("Simulation:save(): client.writeData() failed")
		return false
	end

	return true
end
function Simulation:load(filename)
	util.log("Simulation:load(): filename=%s", filename)

	local loadedStateStr = client.readData(filename)
	if not loadedStateStr then
		return false
	end

	local loadedState = json.decode(loadedStateStr)

	if loadedState.saveVersion and (loadedState.saveVersion > self.state.saveVersion) then
		util.err("Simulation:load(): save version is too new, saveVersion=%d, save.saveVersion=%d",
				   self.state.saveVersion, loadedState.saveVersion)
		return false
	end
	if loadedState.saveVersion and (loadedState.saveVersion < self.state.saveVersion) then
		util.log("Simulation:load(): save version is older, saveVersion=%d, save.saveVersion=%d",
				   self.state.saveVersion, loadedState.saveVersion)
	end

	self.state = loadedState

	return true
end
function Simulation:onSimulationTests()
	self:step()

	local gameBeforeSave = util.toComparable(self.state)
	assert(self:dump("test_dump.sav"))
	assert(self:save("test_save.sav"))
	assert(self:load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = util.toComparable(self.state)
	if gameBeforeSave ~= gameAfterLoad then
		util.err("Simulation:onSimulationTests(): Mismatch between state before save and after load: before=%s, after=%s",
					  gameBeforeSave, gameAfterLoad)
	end
end
function Simulation:runTests()
	util.log("Simulation:runTests()")

	for _, system in pairs(self.systems) do
		if system.onSimulationTests then
			util.log("Simulation:runTests(): running tests for %s", system.SYSTEM_NAME)

			self:destroy()
			self:create()

			system:onSimulationTests()
		end
	end

	self:destroy()

	util.log("Simulation:runTests() complete")
end

function Simulation.new()
	local simulation = {
		["systems"] = {},
		["state"] = {},
		["static"] = {},
		["created"] = false,
	}

	Simulation.__index = Simulation
	setmetatable(simulation, Simulation)

	Simulation.SYSTEM_NAME = "simulation"
	simulation.systems.simulation = simulation

	util.SYSTEM_NAME = "util"
	simulation.systems.util = util

	return simulation
end

util.logLevel = client.state.logLevel

return Simulation
