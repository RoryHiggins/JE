local UtilSys = require("src/engine/util")
local SimulationSys = require("src/engine/simulation")

local WorldSys = {}
WorldSys.createEvents = {}
function WorldSys.create()
	UtilSys.log("WorldSys.create()")

	SimulationSys.state.world = {}

	for _, event in pairs(WorldSys.createEvents) do
		event()
	end

	return SimulationSys.state.world
end
function WorldSys.runTests()
	SimulationSys.create()
	assert(SimulationSys.state.world ~= nil)
end
table.insert(SimulationSys.postCreateEvents, function()
	SimulationSys.state.world = WorldSys.create()
end)

return WorldSys