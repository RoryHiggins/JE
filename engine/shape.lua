local client = require("engine/client")
local Sprite = require("engine/sprite")


local Shape = {}
Shape.SYSTEM_NAME = "Shape"
function Shape:drawPoint(renderable, camera)
	client.drawPoint(renderable, self.untexturedSprite, camera)
end
function Shape:drawLine(renderable, camera)
	client.drawLine(renderable, self.untexturedSprite, camera)
end
function Shape:drawRect(renderable, camera)
	client.drawSprite(renderable, self.untexturedSprite, camera)
end
function Shape:drawTriangle(renderable, camera)
	client.drawTriangle(renderable, self.untexturedSprite, camera)
end
function Shape:onInitialize(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)

	self.untexturedSprite = self.spriteSys:getUntextured()
end


function Shape:onRunTests()
	local screen = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.screen.x2,
		["y2"] = self.simulation.screen.y2,
	}

	local testTriangle = {
		["x1"] = 16,
		["y1"] = 16,
		["x2"] = 24,
		["y2"] = 16,
		["x3"] = 16,
		["y3"] = 32,
		["z"] = -2,

		["r"] = 0,
		["g"] = 1,
		["b"] = 0,
		["a"] = 1,
	}
	self:drawTriangle(testTriangle, screen)

	local testLine = {
		["x"] = 8,
		["y"] = 8,
		["z"] = -3,
		["w"] = -8,
		["h"] = 8,

		["r"] = 1,
		["g"] = 0,
		["b"] = 0,
		["a"] = 1,
	}
	self:drawLine(testLine, screen)

	local testPoint = {
		["x"] = 8,
		["y"] = 16,
		["z"] = -4,

		["r"] = 0,
		["g"] = 0,
		["b"] = 1,
		["a"] = 1,
	}
	self:drawPoint(testPoint, screen)
end


return Shape
