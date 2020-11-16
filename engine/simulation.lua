local log = require("engine/util/log")
local util = require("engine/util/util")
local client = require("engine/client")

local Simulation = {}
Simulation.__index = Simulation
Simulation.SYSTEM_NAME = "simulation"
Simulation.DUMP_FILE = "./game_dump.sav"
Simulation.SAVE_FILE = "./game_save.sav"
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
		log.error("system is not a table, system=%s", util.toComparable(system))
		return {}
	end

	local systemName = system.SYSTEM_NAME

	if type(systemName) ~= "string" then
		log.error("system.SYSTEM_NAME is not valid, system=%s", util.toComparable(system))
		return {}
	end

	local systemInstance = self.systems[systemName]
	if systemInstance == nil then
		log.debug("instantiating system, systemName=%s", systemName)

		system.__index = system
		systemInstance = setmetatable({}, system)
		self.systems[systemName] = systemInstance

		if systemInstance.onInit then
			systemInstance:onInit(self)
		end

		self.systemsOrder[#self.systemsOrder + 1] = systemInstance
	end

	return systemInstance
end
function Simulation:clientStep()
	log.trace("")

	if not client.state.running then
		self.running = false
	end

	-- only update the client if the game is running
	if self.running then
		client.step()
	end

	self.screen = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = client.state.width,
		["y2"] = client.state.height,
	}

	self.fps = client.state.fps
	log.trace("clientStep complete, fps=%d", self.fps)
end
function Simulation:draw()
	log.trace("")

	self:broadcast("onDraw")
end
function Simulation:step()
	log.trace("")

	self:clientStep()

	self:broadcast("onStep")

	self:draw()
end
function Simulation:worldInit()
	log.debug("")

	self.world = {}
	self:broadcast("onWorldInit")
end
function Simulation:init()
	log.debug("")

	math.randomseed(0)

	self.static.saveVersion = 1

	-- clear anything drawn by previous simulation
	client.drawReset()

	self:broadcast("onInit", self)
	self:worldInit({})
end
function Simulation:start()
	log.debug("")

	if not client.state.running then
		log.info("headless mode (no window): simulation will run tests, step once, then stop")
	end

	self:broadcast("onStart")
	self.running = true
end
function Simulation:stop()
	log.debug("")

	self:broadcast("onStop")
	self.running = false
end
function Simulation:save(filename)
	log.debug("filename=%s", filename)

	local save = {
		["saveVersion"] = self.static.saveVersion,
		["world"] = self.world,
	}

	if not client.writeData(filename, util.json.encode(save)) then
		log.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:load(filename)
	log.info("filename=%s", filename)

	local loadedSaveStr = client.readData(filename)
	if not loadedSaveStr then
		return false
	end

	local loadedSave = util.json.decode(loadedSaveStr)

	if loadedSave.saveVersion and (loadedSave.saveVersion > self.static.saveVersion) then
		log.error("save version is too new, saveVersion=%d, save.saveVersion=%d",
				   self.static.saveVersion, loadedSave.saveVersion)
		return false
	end
	if loadedSave.saveVersion and (loadedSave.saveVersion < self.static.saveVersion) then
		log.info("save version is older, saveVersion=%d, save.saveVersion=%d",
				   self.static.saveVersion, loadedSave.saveVersion)
	end

	self.world = loadedSave.world

	return true
end
function Simulation:dump(filename)
	log.debug("filename=%s", filename)

	local dump = {
		["world"] = self.world,
		["static"] = self.static,
		["systems"] = util.getKeys(self.systems),
	}
	if not util.writeDataUncompressed(filename, util.toComparable(dump)) then
		log.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:onRunTests()
	self:init()

	local gameBeforeSave = util.toComparable(self.world)
	assert(self:dump("test_dump.sav"))
	assert(self:save("test_save.sav"))
	assert(self:load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = util.toComparable(self.world)
	if gameBeforeSave ~= gameAfterLoad then
		log.error("Mismatched state before save and after load: before=%s, after=%s",
				   gameBeforeSave, gameAfterLoad)
	end
end
function Simulation:runTests()
	if not client.state.testsEnabled then
		log.info("tests not enabled, skipping")
		return
	end

	local startTimeSeconds = os.clock()

	local logLevelBackup = log.logLevel
	log.logLevel = client.state.testsLogLevel

	log.info("starting")

	local testSuitesCount = 0

	for _, system in pairs(self.systems) do
		if system.onRunTests and system.SYSTEM_NAME ~= "simulation" then
			log.info("running tests for %s", system.SYSTEM_NAME)

			self:init()
			testSuitesCount = testSuitesCount + (system:onRunTests() or 1)
		end
	end

	-- clear any garbage left by the last test
	self:init()

	log.logLevel = logLevelBackup

	local endTimeSeconds = os.clock()
	local testTimeSeconds = endTimeSeconds - startTimeSeconds
	log.info("complete, testSuitesCount=%d, testTimeSeconds=%.2f",
				  testSuitesCount, testTimeSeconds)
end
function Simulation:run(...)
	self.args = {...}
	log.debug("arguments: %s", util.toComparable(self.args))

	local startTimeSeconds = os.clock()
	local logLevelBackup = log.logLevel
	log.logLevel = client.state.logLevel

	self:init()
	self:runTests()

	self:start()

	while self.running do
		self:step()
	end

	self:stop()

	self:save(self.SAVE_FILE)
	self:dump(self.DUMP_FILE)

	log.logLevel = logLevelBackup

	local endTimeSeconds = os.clock()
	local runTimeSeconds = endTimeSeconds - startTimeSeconds
	log.info("complete, runTimeSeconds=%.2f", runTimeSeconds)
end

function Simulation.new()
	local simulation = {
		["systems"] = nil,  -- populated below as it is self-referential
		["systemsOrder"] = nil,  -- populated below as it is self-referential
		["static"] = {},
		["world"] = {},
		["running"] = false,
		["screen"] = {
			["x1"] = 0,
			["y1"] = 0,
			["x2"] = 0,
			["y2"] = 0,
		},
		["fps"] = 0,
	}
	simulation.systems = {
		["simulation"] = nil,
		["util"] = util,
		["client"] = client,
	}
	simulation.systemsOrder = {
		simulation,
		util,
		client,
	}

	setmetatable(simulation, Simulation)

	return simulation
end

return Simulation
