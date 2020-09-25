local Entity = require("engine/entity")

local Camera = {}
Camera.SYSTEM_NAME = "camera"
function Camera:onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.state.camera = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.screen.x2,
		["y2"] = self.simulation.screen.y2,
	}
end
function Camera:onSimulationDraw(screen)
	local camera = self.simulation.state.camera

	local cameraTarget = self.entitySys:find("cameraTarget")
	if cameraTarget then
		camera.x1 = math.floor(cameraTarget.x + (cameraTarget.w / 2) - ((camera.x2 - camera.x1) / 2))
		camera.y1 = math.floor(cameraTarget.y + (cameraTarget.h / 2) - ((camera.y2 - camera.y1) / 2))
	end

	camera.x2 = camera.x1 + screen.x2
	camera.y2 = camera.y1 + screen.y2

	self.simulation:broadcast("onCameraDraw", camera)
end
function Camera:onRunTests()
	assert(self.simulation.state.camera ~= nil)
	self:onSimulationDraw(self.simulation.screen)
end

return Camera