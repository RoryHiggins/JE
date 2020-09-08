local client = require("src/engine/client")
local Sprite = require("src/engine/sprite")


local Shape = {}
Shape.SYSTEM_NAME = "Shape"
function Shape:drawPoint(renderable, camera)
	client.drawPoint(renderable, self.flatColorSprite, camera)
end
function Shape:drawLine(renderable, camera)
	client.drawLine(renderable, self.flatColorSprite, camera)
end
function Shape:drawRect(renderable, camera)
	client.drawSprite(renderable, self.flatColorSprite, camera)
end
function Shape:drawTriangle(renderable, camera)
	client.drawTriangle(renderable, self.flatColorSprite, camera)
end
function Shape:onSimulationCreate(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)

	self.flatColorSprite = self.spriteSys:addSprite("flatColor", 0, 0, 1, 1)
end

return Shape
