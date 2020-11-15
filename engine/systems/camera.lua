local Entity = require("engine/systems/entity")

local Camera = {}
Camera.SYSTEM_NAME = "camera"
function Camera:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
end
function Camera:onWorldInit()
	self.simulation.world.camera = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.screen.x2,
		["y2"] = self.simulation.screen.y2,
	}
end
function Camera:onDraw()
	local camera = self.simulation.world.camera

	local cameraTarget = self.entitySys:find("cameraTarget")
	if cameraTarget then
		camera.x1 = math.floor(cameraTarget.x + (cameraTarget.w / 2) - ((camera.x2 - camera.x1) / 2))
		camera.y1 = math.floor(cameraTarget.y + (cameraTarget.h / 2) - ((camera.y2 - camera.y1) / 2))
	end

	camera.x2 = camera.x1 + self.simulation.screen.x2
	camera.y2 = camera.y1 + self.simulation.screen.y2

	self.simulation:broadcast("onCameraDraw", camera)
end
function Camera:onRunTests()
	assert(self.simulation.world.camera ~= nil)
	self:onDraw()
end

return Camera