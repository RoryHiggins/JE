local util = require("engine/util")

local World = {}
World.SYSTEM_NAME = "world"
function World:destroy()
	if not self.created then
		return
	end

	self.simulation:broadcast("onWorldDestroy")
	self.created = false
	self.simulation.state.world = nil

	util.debug("")
end
function World:create()
	self:destroy()

	util.debug("")

	local world = {}
	self.simulation.state.world = world
	self.simulation:broadcast("onWorldCreate", world)

	self.created = true
end
function World:onSimulationCreate(simulation)
	self.simulation = simulation
	self:create()
end
function World:onSimulationDestroy()
	self:destroy()
end
function World:onRunTests()
	assert(self.simulation.state.world ~= nil)
end

return World
