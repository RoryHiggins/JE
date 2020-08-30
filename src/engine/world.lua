local Simulation = require("src/engine/simulation")

local World = Simulation.createSystem("world")
function World:onSimulationCreate()
	self.simulation.state.world = {}
end
function World:onSimulationTests()
	assert(self.simulation.state.world ~= nil)
end

return World
