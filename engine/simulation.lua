local util = require("engine/util")
local client = require("engine/client")


local Simulation = {}
Simulation.DUMP_FILE = "./game_dump.sav"
Simulation.SAVE_FILE = "./game_save.sav"
function Simulation:broadcast(event, ...)
	for _, system in pairs(self.systems) do
		local eventHandler = system[event]
		if eventHandler then
			eventHandler(system, ...)
		end
	end
end
function Simulation:addSystem(system)
	if type(system) ~= "table" then
		util.error("system is not a table, system=%s",
				   util.toComparable(system))
		return {}
	end

	local systemName = system.SYSTEM_NAME

	if type(systemName) ~= "string" then
		util.error("system.SYSTEM_NAME is not valid, system=%s",
				   util.toComparable(system))
		return {}
	end

	local systemInstance = self.systems[systemName]
	if systemInstance == nil then
		util.debug("instantiating system, systemName=%s", systemName)

		system.__index = system
		systemInstance = setmetatable({}, system)
		self.systems[systemName] = systemInstance
		self.systemsCreated[systemName] = false
	end

	if self.created and not self.systemsCreated[systemName] then
		util.debug("creating systemInstance, systemName=%s", systemName)

		-- marking as created first, to prevent recursion with circular dependencies
		self.systemsCreated[systemName] = true
		if systemInstance.onSimulationCreate then
			systemInstance:onSimulationCreate(self)
		end
	end

	return systemInstance
end
function Simulation.isRunning()
	return client.state.running
end
function Simulation:draw()
	util.trace("")

	self:broadcast("onSimulationDraw", self.screen)
end
function Simulation:stepClient()
	util.trace("stepClient")

	client.step()

	self.screen = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = client.state.width,
		["y2"] = client.state.height,
	}

	self.fps = client.state.fps
	util.trace("stepClient complete, fps=%d", self.fps)
end
function Simulation:step()
	util.trace("")

	self:broadcast("onSimulationStep")
end
function Simulation:destroy()
	if not self.created then
		return
	end

	self:broadcast("onSimulationDestroy")
	self.state = {}

	for systemName, _ in pairs(self.systems) do
		self.systemsCreated[systemName] = false
	end

	self.created = false
	self.started = false

	util.debug("")
end
function Simulation:create()
	self:destroy()

	util.debug("")

	math.randomseed(0)

	self.created = true
	self.started = false
	self.state.saveVersion = 1

	-- clear anything drawn by previous simulation
	client.drawReset()

	for _, system in pairs(self.systems) do
		util.trace("system.SYSTEM_NAME=%s, created=%s", system.SYSTEM_NAME, self.systemsCreated[system.SYSTEM_NAME])
		if not self.systemsCreated[system.SYSTEM_NAME] then
			util.debug("creating system, system.SYSTEM_NAME=%s", system.SYSTEM_NAME)

			-- marking as created first, to prevent recursion with circular dependencies
			self.systemsCreated[system.SYSTEM_NAME] = true

			if system.onSimulationCreate then
				system:onSimulationCreate(self)
			end
		end
	end
end
function Simulation:start()
	util.info("")

	if not self.created then
		util.warn("simulation not created yet")
		self:create()
	end

	if self.started then
		util.warn("simulation already started")
		return
	end

	self.started = true

	for _, system in pairs(self.systems) do
		if system.onSimulationStart then
			system:onSimulationStart(self)
		end
	end
end
function Simulation:finish()
	util.debug("")

	if not self.created then
		util.warn("simulation not created")
		return
	end

	if not self.started then
		util.warn("simulation not started")
		return
	end

	for _, system in pairs(self.systems) do
		if system.onSimulationFinish then
			system:onSimulationFinish(self)
		end
	end

	self.started = false
end
function Simulation:save(filename)
	util.debug("filename=%s", filename)

	if not client.writeData(filename, util.json.encode(self.state)) then
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

	local loadedState = util.json.decode(loadedStateStr)

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
function Simulation:onRunTests()
	self:create()
	self:step()
	self:draw()

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
	local startTimeSeconds = os.clock()

	local logLevelBackup = util.logLevel
	util.logLevel = client.state.testsLogLevel

	util.info("starting")

	local testSuitesCount = 0

	-- create once to let systems add dependencies
	self:create()
	self:destroy()

	for _, system in pairs(self.systems) do
		if system.onRunTests and system.SYSTEM_NAME ~= "simulation" then
			util.info("running tests for %s", system.SYSTEM_NAME)

			self:destroy()
			self:create()

			testSuitesCount = testSuitesCount + (system:onRunTests() or 1)
		end
	end

	self:destroy()

	util.logLevel = logLevelBackup

	local endTimeSeconds = os.clock()
	local testTimeSeconds = endTimeSeconds - startTimeSeconds
	util.info("complete, testSuitesCount=%d, testTimeSeconds=%.2f",
				  testSuitesCount, testTimeSeconds)
end
function Simulation:run()
	local startTimeSeconds = os.clock()
	-- self:stepClient()
	local logLevelBackup = util.logLevel
	util.logLevel = client.state.logLevel

	if client.state.testsEnabled then
		self:runTests()
	end

	self:create()
	self:start()

	while self:isRunning() do
		self:stepClient()
		self:step()
		self:draw()
	end

	self:finish()

	-- self:save(self.SAVE_FILE)
	self:dump(self.DUMP_FILE)
	self:destroy()

	util.logLevel = logLevelBackup

	local endTimeSeconds = os.clock()
	local runTimeSeconds = endTimeSeconds - startTimeSeconds
	util.info("complete, runTimeSeconds=%.2f", runTimeSeconds)
end

function Simulation.new()
	local simulation = {
		["systems"] = {},
		["systemsCreated"] = {},
		["state"] = {},
		["static"] = {},
		["created"] = false,
		["started"] = false,
		["screen"] = {
			["x1"] = 0,
			["y1"] = 0,
			["x2"] = 0,
			["y2"] = 0,
		},
		["fps"] = 0,
	}

	Simulation.__index = Simulation
	setmetatable(simulation, Simulation)

	Simulation.SYSTEM_NAME = "simulation"
	simulation.systems.simulation = simulation

	util.SYSTEM_NAME = "util"
	simulation.systems.util = util

	client.SYSTEM_NAME = "client"
	simulation.systems.client = client

	return simulation
end

return Simulation
