local util = require("engine/util/util")
local log = require("engine/util/log")
local client = require("engine/client/client")
local Sprite = require("engine/systems/sprite")

local Shape = {}
Shape.SYSTEM_NAME = "shape"
function Shape:drawPoint(renderable, camera)
	client.drawPoint(renderable, self.untexturedSprite, camera)
end
function Shape:drawLine(renderable, camera)
	client.drawLine(renderable, self.untexturedSprite, camera)
end
function Shape:drawRect(renderable, camera, outline)
	-- TODO move this to the client level with support for x1/x2/y1/y2
	if (renderable.w == nil) or (renderable.h == nil) then
		log.assert(renderable.x1 ~= nil)
		log.assert(renderable.y1 ~= nil)
		log.assert(renderable.x2 ~= nil)
		log.assert(renderable.y2 ~= nil)

		renderable = util.deepcopy(renderable)
		renderable.x = renderable.x1
		renderable.y = renderable.y1
		renderable.w = renderable.x2 - renderable.x1
		renderable.h = renderable.y2 - renderable.y1
		renderable.x1 = nil
		renderable.y1 = nil
		renderable.x2 = nil
		renderable.y2 = nil
	end

	if (renderable.w <= 2) or (renderable.h <= 2) then
		outline = true
	end

	if not outline then
		client.drawSprite(renderable, self.untexturedSprite, camera)
		return
	end

	local srcRenderable = renderable
	renderable = util.deepcopy(srcRenderable)

	renderable.h = 0
	client.drawLine(renderable, self.untexturedSprite, camera)

	renderable.y = renderable.y + srcRenderable.h - 1
	client.drawLine(renderable, self.untexturedSprite, camera)

	renderable.y = renderable.y - srcRenderable.h + 1
	renderable.h = srcRenderable.h

	renderable.w = 0
	renderable.y = renderable.y + 1
	renderable.h = renderable.h - 2
	client.drawLine(renderable, self.untexturedSprite, camera)

	renderable.x = renderable.x + srcRenderable.w - 1
	client.drawLine(renderable, self.untexturedSprite, camera)
end
function Shape:drawTriangle(renderable, camera)
	client.drawTriangle(renderable, self.untexturedSprite, camera)
end
function Shape:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)

	self.untexturedSprite = self.spriteSys:getUntextured()
end


function Shape:onRunTests()
	local screen = {
		["x1"] = 0,
		["y1"] = 0,
		["x2"] = self.simulation.input.screen.x2,
		["y2"] = self.simulation.input.screen.y2,
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

	local testRect = {
		["x"] = 8,
		["y"] = 8,
		["z"] = -3,
		["w"] = 8,
		["h"] = 8,

		["r"] = 1,
		["g"] = 0,
		["b"] = 0,
		["a"] = 1,
	}
	self:drawRect(testRect, screen, --[[outline--]] false)
	self:drawRect(testRect, screen, --[[outline--]] true)

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
