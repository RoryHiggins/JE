local Entity = require("src/engine/entity")

local Screen = {}
Screen.SYSTEM_NAME = "screen"
function Screen:onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

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
function Screen:onSimulationRunTests()
	assert(self.simulation.state.screen ~= nil)
	self:onSimulationDraw()
end

return Screen