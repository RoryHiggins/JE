local util = require("engine/util/util")
local Shape = require("engine/systems/shape")
local Sprite = require("engine/systems/sprite")

local Background = {}
Background.SYSTEM_NAME = "background"
function Background:onInit(simulation)
	self.simulation = simulation
	self.shapeSys = self.simulation:addSystem(Shape)
	self.spriteSys = self.simulation:addSystem(Sprite)

	self.background = {
		["r"] = 1,
		["g"] = 1,
		["b"] = 1,
		["a"] = 1,
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.input.screen.x2,
		["y2"] = self.simulation.input.screen.y2,
		["z"] = 1000,
	}
	self.backgroundDefaults = {
		["r"] = 0.3,
		["g"] = 0.2,
		["b"] = 0.2,
	}
	self:resetColor()
end
function Background:onWorldInit()
	self:resetColor()
end
function Background:resetColor()
	self:setColor(self.backgroundDefaults.r, self.backgroundDefaults.g, self.backgroundDefaults.b)
end
function Background:getColor()
	local color = self.background
	return color.r, color.g, color.b, color.a
end
function Background:setColor(r, g, b)
	self.background.r = r
	self.background.g = g
	self.background.b = b
end
function Background:onDraw()
	self.background.x2 = self.simulation.input.screen.x2
	self.background.y2 = self.simulation.input.screen.y2
	self.shapeSys:drawRect(self.background, self.simulation.input.screen, false)
end

return Background
