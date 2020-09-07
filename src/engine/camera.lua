local Entity = require("src/engine/entity")

local Camera = {}
Camera.SYSTEM_NAME = "camera"
function Camera:onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.state.camera = {
		["x"] = 0,
		["y"] = 0,
		["w"] = 160,
		["h"] = 120,
	}
end
function Camera:onSimulationDraw()
	local camera = self.simulation.state.camera

	local cameraTarget = self.entitySys:find("cameraTarget")
	if cameraTarget then
		camera.x = math.floor(cameraTarget.x + (cameraTarget.w / 2) - (camera.w / 2))
		camera.y = math.floor(cameraTarget.y + (cameraTarget.h / 2) - (camera.h / 2))
	end

	self.simulation:broadcast("onCameraDraw", camera)

	local screen = {
		["x"] = 0,
		["y"] = 0,
		["w"] = camera.w,
		["h"] = camera.h,
	}
	self.simulation:broadcast("onScreenDraw", screen)
end
function Camera:onSimulationRunTests()
	assert(self.simulation.state.camera ~= nil)
	self:onSimulationDraw()
end

return Camera