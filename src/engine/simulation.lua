local json = require("lib/json")
local UtilSys = require("src/engine/util")
local ClientSys = require("src/engine/client")

local SimulationSys = {}
SimulationSys.SAVE_VERSION = 1
SimulationSys.createEvents = {}
SimulationSys.postCreateEvents = {}
SimulationSys.destroyEvents = {}
SimulationSys.stepEvents = {}
SimulationSys.drawEvents = {}
SimulationSys.state = {}
SimulationSys.static = {}
SimulationSys.inputs = {
	["left"] = false,
	["right"] = false,
	["up"] = false,
	["down"] = false,

	["a"] = false,
	["b"] = false,

	["restart"] = false,
}
function SimulationSys.isRunning()
	return SimulationSys.state.created and ClientSys.isRunning()
end
function SimulationSys.step()
	local events = SimulationSys.stepEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i]()
	end

	ClientSys.updateInputs(SimulationSys.inputs)

	SimulationSys.draw()

	ClientSys.step()
end
function SimulationSys.draw()
	local events = SimulationSys.drawEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i]()
	end
end
function SimulationSys.destroy()
	UtilSys.log("SimulationSys.destroy()")

	if not SimulationSys.state.created then
		return
	end

	for _, event in pairs(SimulationSys.destroyEvents) do
		event()
	end

	for key, _ in pairs(SimulationSys.state) do
		SimulationSys.state[key] = nil
	end
end
function SimulationSys.create()
	if SimulationSys.state.created then
		SimulationSys.destroy()
	end

	UtilSys.log("SimulationSys.create()")

	SimulationSys.state.created = true
	SimulationSys.state.saveVersion = SimulationSys.SAVE_VERSION

	for _, event in pairs(SimulationSys.createEvents) do
		event()
	end

	for _, event in pairs(SimulationSys.postCreateEvents) do
		event()
	end

	return SimulationSys.state
end
function SimulationSys.dump(filename)
	UtilSys.log("SimulationSys.dump(): filename=%s", filename)

	local dump = {
		["state"] = SimulationSys.state,
		["static"] = SimulationSys.static,
	}
	local success = ClientSys.writeDataRaw(filename, UtilSys.toComparable(dump))

	if not success then
		UtilSys.log("SimulationSys.dump(): ClientSys.writeData() failed")
	end
end
function SimulationSys.save(filename)
	UtilSys.log("SimulationSys.save(): filename=%s", filename)

	local success = ClientSys.writeData(filename, json.encode(SimulationSys.state))

	if not success then
		UtilSys.log("SimulationSys.save(): ClientSys.writeData() failed")
	end
end
function SimulationSys.load(filename)
	UtilSys.log("SimulationSys.load(): filename=%s", filename)

	local loadedSimulation = json.decode(ClientSys.readData(filename))

	if loadedSimulation.saveVersion > SimulationSys.SAVE_VERSION then
		UtilSys.err("SimulationSys.load(): save version is too new, saveVersion=%d, save.saveVersion=%d",
				   SimulationSys.SAVE_VERSION, loadedSimulation.saveVersion)
		return
	end
	if loadedSimulation.saveVersion < SimulationSys.SAVE_VERSION then
		UtilSys.log("SimulationSys.load(): save version is older, saveVersion=%d, save.saveVersion=%d",
				   SimulationSys.SAVE_VERSION, loadedSimulation.saveVersion)
	end

	SimulationSys.create()
	local state = SimulationSys.state
	for key, val in pairs(loadedSimulation) do
		state[key] = val
	end
end
function SimulationSys.runTests()
	SimulationSys.destroy()
	SimulationSys.create()

	SimulationSys.step()
	SimulationSys.draw()

	local gameBeforeSave = UtilSys.toComparable(SimulationSys.state)
	SimulationSys.dump("test_dump.json")
	SimulationSys.save("test_save.json")
	SimulationSys.load("test_save.json")
	os.remove("test_dump.json")
	os.remove("test_save.json")

	local gameAfterLoad = UtilSys.toComparable(SimulationSys.state)
	if gameBeforeSave ~= gameAfterLoad then
		UtilSys.err("SimulationSys.runTests(): Mismatch between state before save and after load: before=%s, after=%s",
					  gameBeforeSave, gameAfterLoad)
	end
end

return SimulationSys
