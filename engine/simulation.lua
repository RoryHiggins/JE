local util = require("engine/util")
local client = require("engine/client")


local Simulation = {}
Simulation.DUMP_FILE = "./game_dump.sav"
Simulation.SAVE_FILE = "./game_save.sav"
Simulation.STATE_UNINITIALIZED = "uninitialized"
Simulation.STATE_INITALIZED = "initialized"
Simulation.STATE_STARTED = "started"
Simulation.STATE_STOPPED = "stopped"
function Simulation:broadcast(event, ...)
	for _, system in ipairs(self.systemsOrder) do
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

		-- marking as initialized first, to prevent recursion with circular dependencies
		if systemInstance.onInitialize then
			systemInstance:onInitialize(self)
		end

		self.systemsOrder[#self.systemsOrder + 1] = systemInstance
	end

	return systemInstance
end
function Simulation.isRunning()
	return client.state.running
end
function Simulation:draw()
	util.trace("")

	self:broadcast("onDraw", self.screen)
end
function Simulation:clientStep()
	util.trace("clientStep")

	client.step()

	self.screen = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = client.state.width,
		["y2"] = client.state.height,
	}

	self.fps = client.state.fps
	util.trace("clientStep complete, fps=%d", self.fps)
end
function Simulation:step()
	util.trace("")

	self:broadcast("onStep")
end
function Simulation:destroy()
	if not self.initialized then
		return
	end

	self:broadcast("onDestroy", self)
	self.world = {}

	self.initialized = false
	self.started = false

	util.debug("")
end
function Simulation:worldInitialize()
	util.debug("")

	self.world = {}
	self:broadcast("onWorldInitialize", self.world)
	self:broadcast("onWorldStart", self.world)
end
function Simulation:initialize()
	self:destroy()

	util.debug("")

	math.randomseed(0)

	self.started = false
	self.static.saveVersion = 1

	-- clear anything drawn by previous simulation
	client.drawReset()

	self:broadcast("onInitialize", self)

	self:worldInitialize()

	self.initialized = true

	self:broadcast("onStart", self)
	self.started = true
end
function Simulation:start()
	util.info("")

	if not self.initialized then
		util.warn("simulation not initialized yet")
		self:initialize()
	end

	if self.started then
		util.warn("simulation already started")
		return
	end

	self.started = true
end
function Simulation:stop()
	util.debug("")

	if not self.initialized then
		util.warn("simulation not initialized")
		return
	end

	if not self.started then
		util.warn("simulation not started")
		return
	end

	self:broadcast("onStop")
	self.started = false
end
function Simulation:save(filename)
	util.debug("filename=%s", filename)

	local save = {
		["saveVersion"] = self.static.saveVersion,
		["world"] = self.world,
	}

	if not client.writeData(filename, util.json.encode(save)) then
		util.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:load(filename)
	util.info("filename=%s", filename)

	local loadedSaveStr = client.readData(filename)
	if not loadedSaveStr then
		return false
	end

	local loadedSave = util.json.decode(loadedSaveStr)

	if loadedSave.saveVersion and (loadedSave.saveVersion > self.static.saveVersion) then
		util.error("save version is too new, saveVersion=%d, save.saveVersion=%d",
				   self.static.saveVersion, loadedSave.saveVersion)
		return false
	end
	if loadedSave.saveVersion and (loadedSave.saveVersion < self.static.saveVersion) then
		util.info("save version is older, saveVersion=%d, save.saveVersion=%d",
				   self.static.saveVersion, loadedSave.saveVersion)
	end

	self.world = loadedSave.world

	return true
end
function Simulation:dump(filename)
	util.debug("filename=%s", filename)

	local dump = {
		["world"] = self.world,
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
	self:initialize()
	self:step()
	self:draw()

	local gameBeforeSave = util.toComparable(self.world)
	assert(self:dump("test_dump.sav"))
	assert(self:save("test_save.sav"))
	assert(self:load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = util.toComparable(self.world)
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

	-- initialize once to let systems add dependencies
	self:initialize()
	self:destroy()

	for _, system in pairs(self.systems) do
		if system.onRunTests and system.SYSTEM_NAME ~= "simulation" then
			util.info("running tests for %s", system.SYSTEM_NAME)

			self:destroy()
			self:initialize()

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
	local logLevelBackup = util.logLevel
	util.logLevel = client.state.logLevel

	if client.state.testsEnabled then
		self:runTests()
	end

	self:initialize()
	self:start()

	while self:isRunning() do
		self:clientStep()
		self:step()
		self:draw()
	end

	self:stop()

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
		["systemsOrder"] = {},
		["world"] = {},
		["static"] = {},
		["initialized"] = false,
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
