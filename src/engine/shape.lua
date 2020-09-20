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

	self.flatColorSprite = self.spriteSys:addSprite("flatColor", 0, 0, 0, 0)
end


function Shape:onRunTests()
	local screen = {
		["x"] = 0,
		["y"] = 0,
		["w"] = self.simulation.screen.w,
		["h"] = self.simulation.screen.h,
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
