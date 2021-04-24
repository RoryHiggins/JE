local log = require("engine/util/log")
local util = require("engine/util/util")
local client = require("engine/client/client")

local Simulation = {}
Simulation.__index = Simulation
Simulation.SYSTEM_NAME = "simulation"
Simulation.DUMP_FILE = "./game_dump.json"
Simulation.SAVE_FILE = "./game_save.sav"
function Simulation:broadcast(event, tolerate_errors, ...)
	for _, system in ipairs(self.private.eventListeners) do
		local eventHandler = system[event]
		if eventHandler then
			if tolerate_errors then
				log.protectedCall(eventHandler, system, ...)
			else
				eventHandler(system, ...)
			end
		end
	end
end
function Simulation:addSystem(system)
	if type(system) ~= "table" then
		log.error("system is not a table, system=%s", util.getComparable(system))
		return {}
	end

	local systemName = system.SYSTEM_NAME

	if type(systemName) ~= "string" then
		log.error("system.SYSTEM_NAME is not valid, system=%s", util.getComparable(system))
		return {}
	end

	local systemInstance = self.private.systems[systemName]
	if systemInstance == nil then
		log.debug("instantiating system, systemName=%s", systemName)

		system.__index = system
		systemInstance = setmetatable({}, system)
		self.private.systems[systemName] = systemInstance

		if systemInstance.onInit then
			systemInstance:onInit(self)
		end

		self.private.eventListeners[#self.private.eventListeners + 1] = systemInstance
	end

	return systemInstance
end
function Simulation:getSystem(systemName)
	if type(systemName) ~= "string" then
		log.debug("systemName is not registered, systemName=%s", systemName)
		return {}
	end

	return self.private.systems[systemName]
end
function Simulation:step()
	log.trace("")

	if not client.state.running then
		self.private.running = false
	end

	if not self.private.running then
		return
	end

	-- only update the client if the game is running
	client.step()
	log.trace("client.step complete, fps=%d", self.input.fps)

	-- consume input updates from client
	self.input.screen.x2 = client.state.width
	self.input.screen.y2 = client.state.height
	self.input.fps = client.state.fps

	self:broadcast("onStep", true)
end
function Simulation:draw()
	log.trace("")

	self:broadcast("onDraw", true)
end
function Simulation:worldInit()
	log.debug("")

	self.state.world = {}
	self:broadcast("onWorldInit", false)
end
function Simulation:init()
	log.debug("arguments: %s", util.getComparable(self.private.args))

	math.randomseed(0)

	-- clear anything drawn by previous simulation
	client.drawReset()

	self.private.running = false

	self:broadcast("onInit", false, self)
	self:worldInit()
end
function Simulation:start()
	log.debug("")

	if not client.state.running then
		log.info("headless mode (no window): simulation will run tests, step once, then stop")
	end

	if not self.private.running then
		self.private.running = true
		self:broadcast("onStart", false)
	end
end
function Simulation:stop()
	log.debug("")

	if self.private.running then
		self:broadcast("onStop", true)
		self.private.running = false
	end
end
function Simulation:save(filename)
	log.debug("filename=%s", filename)

	local save = {
		["saveVersion"] = self.constants.saveVersion,
		["state"] = self.state,
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

	if loadedSave.saveVersion and (loadedSave.saveVersion > self.constants.saveVersion) then
		log.error("save version is too new, saveVersion=%d, save.saveVersion=%d",
				   self.constants.saveVersion, loadedSave.saveVersion)
		return false
	end
	if loadedSave.saveVersion and (loadedSave.saveVersion < self.constants.saveVersion) then
		log.info("save version is older, saveVersion=%d, save.saveVersion=%d",
				   self.constants.saveVersion, loadedSave.saveVersion)
	end

	self.state = loadedSave.state
	self:broadcast("onLoadState", false)

	return true
end
function Simulation:dump(filename)
	log.debug("filename=%s", filename)

	local dump = {
		["state"] = self.state,
		["constants"] = self.constants,
		["systems"] = util.tableGetKeys(self.private.systems),
	}
	if not util.writeDataUncompressed(filename, util.getComparable(dump)) then
		log.error("client.writeData() failed")
		return false
	end

	return true
end
function Simulation:onRunTests()
	self:init()

	local gameBeforeSave = util.getComparable(self.state.world)
	log.assert(self:dump("test_dump.sav"))
	log.assert(self:save("test_save.sav"))
	log.assert(self:load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = util.getComparable(self.state.world)
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

	log.pushLogLevel(client.state.testsLogLevel)
	log.info("starting")

	local testSuitesCount = 0
	local startTimeSeconds = os.clock()

	for _, system in pairs(self.private.systems) do
		if system.onRunTests and system.SYSTEM_NAME ~= "simulation" then
			log.info("running tests for %s", system.SYSTEM_NAME)

			self:init()
			testSuitesCount = testSuitesCount + (system:onRunTests() or 1)
		end
	end

	local endTimeSeconds = os.clock()
	local testTimeSeconds = endTimeSeconds - startTimeSeconds

	log.popLogLevel()
	log.info("complete, testSuitesCount=%d, testTimeSeconds=%.2f",
				  testSuitesCount, testTimeSeconds)
end
function Simulation:run(...)
	local startTimeSeconds = os.clock()

	self.private.args = {...}

	log.pushLogLevel(client.state.logLevel)

	if util.tableHasValue(self.private.args, "--debug") then
		log.enableDebugger()
	end

	local function runInternal()
		self:runTests()

		self:init()
		self:start()

		while self.private.running do
			self:step()
			self:draw()
		end

		self:stop()

		self:dump(self.DUMP_FILE)
	end

	log.protectedCall(runInternal)

	log.info("runTimeSeconds=%.2f", os.clock() - startTimeSeconds)
	log.popLogLevel()
end

function Simulation.new()
	local simulation = {
		-- State internal to the Simulation instance
		["private"] = {
			-- whether the game is currently running
			["running"] = false,
			["systems"] = {},
			["eventListeners"] = {},
			["args"] = {},
			["startTimeSeconds"] = 0,
			["endTimeSeconds"] = 0,
		},

		-- Constants defining the behavior of the simulation
		["constants"] = {
			["saveVersion"] = 1,
			["developerDebugging"] = false,
		},

		-- Input from the client to the simulation step (screen, fps, keyboard/controller input status, random seed, etc)
		-- Should be the only entirely non-deterministic state that can influence simulation.state
		["input"] = {
			["screen"] = {
				["x1"] = 0,
				["y1"] = 0,
				["x2"] = 0,
				["y2"] = 0,
			},
			["fps"] = 0,
		},

		-- Current simulation state
		["state"] = {
			["world"] = {},
		},
	}
	-- must assign recursive references after initializer
	local eventListeners = simulation.private.eventListeners
	eventListeners[#eventListeners + 1] = simulation

	-- add pseudo-systems to systems, for unit tests
	local systems = simulation.private.systems
	systems["log"] = log
	systems["util"] = util
	systems["client"] = client

	setmetatable(simulation, Simulation)

	return simulation
end

return Simulation
