local json = require("lib/json")
local UtilSys = require("src/engine/util")
local client = require("src/engine/client")

local simulation = {}
simulation.SAVE_VERSION = 1
simulation.createEvents = {}
simulation.postCreateEvents = {}
simulation.destroyEvents = {}
simulation.stepEvents = {}
simulation.drawEvents = {}
simulation.state = {}
simulation.static = {}
function simulation.isRunning()
	return simulation.state.running and client.state.running
end
function simulation.step()
	local events = simulation.stepEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i]()
	end

	simulation.draw()
end
function simulation.draw()
	local events = simulation.drawEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i]()
	end
end
function simulation.destroy()
	UtilSys.log("simulation.destroy()")

	if not simulation.state.running then
		return
	end

	for _, event in pairs(simulation.destroyEvents) do
		event()
	end

	for key, _ in pairs(simulation.state) do
		simulation.state[key] = nil
	end
end
function simulation.create()
	if simulation.state.running then
		simulation.destroy()
	end

	UtilSys.log("simulation.create()")

	simulation.state.running = true
	simulation.state.saveVersion = simulation.SAVE_VERSION

	for _, event in pairs(simulation.createEvents) do
		event()
	end

	for _, event in pairs(simulation.postCreateEvents) do
		event()
	end

	return simulation.state
end
function simulation.dump(filename)
	UtilSys.log("simulation.dump(): filename=%s", filename)

	local dump = {
		["state"] = simulation.state,
		["static"] = simulation.static,
	}
	if not UtilSys.writeDataUncompressed(filename, UtilSys.toComparable(dump)) then
		UtilSys.log("simulation.dump(): client.writeData() failed")
		return false
	end

	return true
end
function simulation.save(filename)
	UtilSys.log("simulation.save(): filename=%s", filename)

	if not client.writeData(filename, json.encode(simulation.state)) then
		UtilSys.log("simulation.save(): client.writeData() failed")
		return false
	end

	return true
end
function simulation.load(filename)
	UtilSys.log("simulation.load(): filename=%s", filename)

	local loadedSimulationStr = client.readData(filename)
	if not loadedSimulationStr then
		return false
	end

	local loadedSimulation = json.decode(loadedSimulationStr)

	if loadedSimulation.saveVersion > simulation.SAVE_VERSION then
		UtilSys.err("simulation.load(): save version is too new, saveVersion=%d, save.saveVersion=%d",
				   simulation.SAVE_VERSION, loadedSimulation.saveVersion)
		return false
	end
	if loadedSimulation.saveVersion < simulation.SAVE_VERSION then
		UtilSys.log("simulation.load(): save version is older, saveVersion=%d, save.saveVersion=%d",
				   simulation.SAVE_VERSION, loadedSimulation.saveVersion)
	end

	simulation.create()
	local state = simulation.state
	for key, val in pairs(loadedSimulation) do
		state[key] = val
	end

	return true
end
function simulation.runTests()
	simulation.destroy()
	simulation.create()

	simulation.step()
	simulation.draw()

	local gameBeforeSave = UtilSys.toComparable(simulation.state)
	assert(simulation.dump("test_dump.sav"))
	assert(simulation.save("test_save.sav"))
	assert(simulation.load("test_save.sav"))
	os.remove("test_dump.sav")
	os.remove("test_save.sav")

	local gameAfterLoad = UtilSys.toComparable(simulation.state)
	if gameBeforeSave ~= gameAfterLoad then
		UtilSys.err("simulation.runTests(): Mismatch between state before save and after load: before=%s, after=%s",
					  gameBeforeSave, gameAfterLoad)
	end
end

return simulation
