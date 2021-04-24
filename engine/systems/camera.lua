local log = require("engine/util/log")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")

local Camera = {}
Camera.SYSTEM_NAME = "camera"
function Camera:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.inputSys = self.simulation:addSystem(Input)
end
function Camera:onWorldInit()
	self.simulation.state.world.camera = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.input.screen.x2,
		["y2"] = self.simulation.input.screen.y2,
		["mouseX"] = 0,
		["mouseY"] = 0,
	}
end
function Camera:onDraw()
	local camera = self.simulation.state.world.camera

	local cameraTarget = self.entitySys:find("cameraTarget")
	if cameraTarget then
		camera.x1 = math.floor(cameraTarget.x + (cameraTarget.w / 2) - ((camera.x2 - camera.x1) / 2))
		camera.y1 = math.floor(cameraTarget.y + (cameraTarget.h / 2) - ((camera.y2 - camera.y1) / 2))
	end

	camera.x2 = camera.x1 + self.simulation.input.screen.x2
	camera.y2 = camera.y1 + self.simulation.input.screen.y2

	local mouseX, mouseY = self.inputSys:getMousePos()
	camera.mouseX = mouseX - camera.x1
	camera.mouseY = mouseY - camera.y1

	self.simulation:broadcast("onCameraDraw", true, camera)
end
function Camera:onRunTests()
	log.assert(self.simulation.state.world.camera ~= nil)
	self:onDraw()
end

return Camera
