local util = require("src/engine/util")
local System = require("src/engine/system")

local World = System.new("world")
function World:destroy()
	if not self.created then
		return
	end

	util.log("World:destroy()")

	self.simulation:broadcast("onWorldDestroy")
	self.created = false
	self.simulation.state.world = nil
end
function World:create()
	self:destroy()

	util.log("World:create()")

	self.simulation.state.world = {}
	self.simulation:broadcast("onWorldCreate")

	self.created = true
end
function World:onSimulationCreate()
	self:create()
end
function World:onSimulationTests()
	assert(self.simulation.state.world ~= nil)
end

return World
