local System = require("src/engine/system")
local Entity = require("src/engine/entity")

local Screen = System.new("screen")
function Screen:onSimulationCreate()
	self:addDependencies(Entity)

	self.simulation.state.screen = {
		["x"] = 0,
		["y"] = 0,
	}
end
function Screen:onSimulationDraw()
	local screen = self.simulation.state.screen
	local screenTarget = self.entitySys:find("screenTarget")
	if screenTarget then
		screen.x = screenTarget.x
		screen.y = screenTarget.y
	end

	self.simulation:broadcast("onScreenDraw", screen)
end
function Screen:onSimulationTests()
	assert(self.simulation.state.screen ~= nil)
	self:onSimulationDraw()
end

return Screen