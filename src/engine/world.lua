local util = require("src/engine/util")

local World = {}
World.SYSTEM_NAME = "world"
function World:destroy()
	if not self.created then
		return
	end

	util.info("")

	self.simulation:broadcast("onWorldDestroy")
	self.created = false
	self.simulation.state.world = nil
end
function World:create()
	self:destroy()

	util.info("")

	local world = {}
	self.simulation.state.world = world
	self.simulation:broadcast("onWorldCreate", world)

	self.created = true
end
function World:onSimulationCreate(simulation)
	self.simulation = simulation
end
function World:onSimulationDestroy()
	self:destroy()
end
function World:onSimulationRunTests()
	assert(self.simulation.state.world ~= nil)
end

return World
