local System = require("src/engine/system")

local World = System.new("world")
function World:onSimulationCreate()
	self.simulation.state.world = {}
end
function World:onSimulationTests()
	assert(self.simulation.state.world ~= nil)
end

return World
