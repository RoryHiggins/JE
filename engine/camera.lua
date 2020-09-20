local Entity = require("engine/entity")

local Camera = {}
Camera.SYSTEM_NAME = "camera"
function Camera:onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.state.camera = {
		["x"] = 0,
		["y"] = 0,
		["w"] = self.simulation.screen.w,
		["h"] = self.simulation.screen.h,
	}
end
function Camera:onSimulationDraw(screen)
	local camera = self.simulation.state.camera

	camera.w = screen.w
	camera.h = screen.h

	local cameraTarget = self.entitySys:find("cameraTarget")
	if cameraTarget then
		camera.x = math.floor(cameraTarget.x + (cameraTarget.w / 2) - (camera.w / 2))
		camera.y = math.floor(cameraTarget.y + (cameraTarget.h / 2) - (camera.h / 2))
	end

	self.simulation:broadcast("onCameraDraw", camera)
end
function Camera:onRunTests()
	assert(self.simulation.state.camera ~= nil)
	self:onSimulationDraw(self.simulation.screen)
end

return Camera
