local json = require("lib/json/json")
local util = require("src/engine/util")
local client = require("src/engine/client")


local Simulation = {}
Simulation.DUMP_FILE = ".\\game_dump.sav"
Simulation.SAVE_FILE = ".\\game_save.sav"
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
		util.error("class is not a table, class=%s", util.toComparable(class))
		return {}
	end

	local systemName = class.SYSTEM_NAME

	if type(systemName) ~= "string" then
		util.error("class.SYSTEM_NAME is not valid, class=%s", util.toComparable(class))
		return {}
	end

	local system = self.systems[systemName]
	if system == nil then
		util.debug("instantiating system, systemName=%s", systemName)

		class.__index = class
		system = setmetatable({}, class)
		self.systems[systemName] = system
		self.systemsCreated[systemName] = false
	end

	if self.created and not self.systemsCreated[systemName] then
		util.debug("creating system, systemName=%s", systemName)

		-- marking as created first, to prevent recursion with circular dependencies
		self.systemsCreated[systemName] = true
		if system.onSimulationCreate then
			system:onSimulationCreate(self)
		end
	end

	return system
end
function Simulation.isRunning()
	return client.state.running
end
function Simulation:stepClient()
	client.step()

	self.screen = {
		["x"] = 0,
		["y"] = 0,
		["w"] = client.state.width,
		["h"] = client.state.height,
	}

	self.fps = client.state.fps
end
function Simulation:step()
	-- avoid pumping client during simulation unit tests, in case it's a real client
	if not self.runningTests then
		self:stepClient()
	end

	self:broadcast("onSimulationStep")
	self:broadcast("onSimulationDraw", self.screen)
end
function Simulation:destroy()
	if not self.created then
		return
	end

	util.info("")

	self:broadcast("onSimulationDestroy")
	self.state = {}

	for systemName, _ in pairs(self.systems) do
		self.systemsCreated[systemName] = false
	end

	self.created = false
end
function Simulation:create()
	self:destroy()

	util.info("")

	math.randomseed(0)

	self.created = true
	self.state.saveVersion = 1

	for systemName, system in pairs(self.systems) do
		util.trace("systemName=%s, created=%s", systemName, self.systemsCreated[systemName])
		if not self.systemsCreated[systemName] then
			util.debug("creating system, name=%s", systemName)

			-- marking as created first, to prevent recursion with circular dependencies
			self.systemsCreated[systemName] = true

			if system.onSimulationCreate then
				system:onSimulationCreate(self)
			end
		end
	end
end
function Simulation:save(filename)
	util.debug("filename=%s", filename)

	if not client.writeData(filename, json.encode(self.state)) then
		util.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:load(filename)
	util.info("filename=%s", filename)

	local loadedStateStr = client.readData(filename)
	if not loadedStateStr then
		return false
	end

	local loadedState = json.decode(loadedStateStr)

	if loadedState.saveVersion and (loadedState.saveVersion > self.state.saveVersion) then
		util.error("save version is too new, saveVersion=%d, save.saveVersion=%d",
				   self.state.saveVersion, loadedState.saveVersion)
		return false
	end
	if loadedState.saveVersion and (loadedState.saveVersion < self.state.saveVersion) then
		util.info("save version is older, saveVersion=%d, save.saveVersion=%d",
				   self.state.saveVersion, loadedState.saveVersion)
	end

	self.state = loadedState

	return true
end
function Simulation:dump(filename)
	util.debug("filename=%s", filename)

	local dump = {
		["state"] = self.state,
		["static"] = self.static,
		["systems"] = util.getKeys(self.systems),
	}
	if not util.writeDataUncompressed(filename, util.toComparable(dump)) then
		util.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:onSimulationRunTests()
	self:create()
	self:step()

	local gameBeforeSave = util.toComparable(self.state)
	assert(self:dump("test_dump.sav"))
	assert(self:save("test_save.sav"))
	assert(self:load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = util.toComparable(self.state)
	if gameBeforeSave ~= gameAfterLoad then
		util.error("Mismatched state before save and after load: before=%s, after=%s",
				   gameBeforeSave, gameAfterLoad)
	end

	self:destroy()
end
function Simulation:runTests()
	local logLevelBackup = util.logLevel
	util.logLevel = client.state.testsLogLevel

	util.info("starting")
	self.runningTests = true

	for _, system in pairs(self.systems) do
		if system.onSimulationRunTests then
			util.info("running tests for %s", system.SYSTEM_NAME)

			self:destroy()
			self:create()

			system:onSimulationRunTests()
		end
	end

	self:destroy()

	util.info("complete")
	self.runningTests = false

	util.logLevel = logLevelBackup
end
function Simulation:run()
	self:stepClient()
	util.logLevel = client.state.logLevel

	if client.state.testsEnabled then
		util.info("running tests")
		self:runTests()
	end

	util.info("starting")
	self:create()

	while self:isRunning() do
		self:step()
	end

	util.info("ending")

	-- self:save(self.SAVE_FILE)
	-- self:dump(self.DUMP_FILE)
	self:destroy()
end

function Simulation.new()
	local simulation = {
		["systems"] = {},
		["systemsCreated"] = {},
		["state"] = {},
		["static"] = {},
		["created"] = false,
		["runningTests"] = false,  -- use extremely sparingly
		["screen"] = {
			["x"] = 0,
			["y"] = 0,
			["w"] = 0,
			["h"] = 0,
		},
		["fps"] = 0,
	}

	Simulation.__index = Simulation
	setmetatable(simulation, Simulation)

	Simulation.SYSTEM_NAME = "simulation"
	simulation.systems.simulation = simulation

	util.SYSTEM_NAME = "util"
	simulation.systems.util = util

	return simulation
end

return Simulation
