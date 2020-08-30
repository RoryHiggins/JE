local UtilSys = require("src/engine/util")
local simulation = require("src/engine/simulation")

local WorldSys = {}
WorldSys.createEvents = {}
function WorldSys.create()
	UtilSys.log("WorldSys.create()")

	simulation.state.world = {}

	for _, event in pairs(WorldSys.createEvents) do
		event()
	end

	return simulation.state.world
end
function WorldSys.runTests()
	simulation.create()
	assert(simulation.state.world ~= nil)
end
table.insert(simulation.postCreateEvents, function()
	simulation.state.world = WorldSys.create()
end)

return WorldSys