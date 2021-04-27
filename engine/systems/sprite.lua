local log = require("engine/util/log")
local util = require("engine/util/util")
local client = require("engine/client/client")
local Entity = require("engine/systems/entity")
local Camera = require("engine/systems/camera")

local Sprite = {}
Sprite.SYSTEM_NAME = "sprite"
function Sprite:addSprite(spriteId, u, v, w, h, r, g, b, a)
	local sprites = self.simulation.constants.sprites
	local sprite = sprites[spriteId]
	if sprite == nil then
		sprite = {
			["spriteId"] = spriteId,
			["u1"] = u,
			["v1"] = v,
			["u2"] = u + w,
			["v2"] = v + h,
			["r"] = r or 1,
			["g"] = g or 1,
			["b"] = b or 1,
			["a"] = a or 1,
		}
		sprites[spriteId] = sprite
	end

	log.trace("sprite=%s", util.getComparable(sprite))

	return sprite
end
function Sprite.draw(_, renderable, sprite, camera)
	client.drawSprite(renderable, sprite, camera)
end
function Sprite:get(spriteId)
	return self.simulation.constants.sprites[spriteId]
end
function Sprite:getUntextured()
	return self.simulation.constants.untexturedSprite
end
function Sprite:getInvalid()
	return self.simulation.constants.invalidSprite
end
function Sprite:attach(entity, sprite)
	entity.spriteId = sprite.spriteId

	self.entitySys:tag(entity, "sprite")
end
function Sprite:detach(entity)
	self.entitySys:untag(entity, "sprite")
	entity.spriteId = nil
end
function Sprite:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.cameraSys = self.simulation:addSystem(Camera)

	self.simulation.constants.sprites = {}
	self.simulation.constants.untexturedSprite = self:addSprite("flatColor", 0, 0, 0, 0)
	self.simulation.constants.invalidSprite = self:addSprite("invalid", 8, 0, 8, 8)
end
function Sprite:onCameraDraw(camera)
	local sprites = self.simulation.constants.sprites

	for _, entity in ipairs(self.entitySys:findAll("sprite")) do
		local sprite = sprites[entity.spriteId]
		if sprite then
			client.drawSprite(entity, sprite, camera)
		else
			log.error("invalid spriteId, entity=%s", util.getComparable(entity))
		end
	end
end
function Sprite:onRunTests()
	local entity = self.entitySys:create()
	local testSprite = self:addSprite("test", 40, 0, 8, 8)

	log.assert(self:get("test") == testSprite)

	self:attach(entity, testSprite)
	log.assert(entity.spriteId == "test")
	log.assert(entity.tags.sprite)

	self.simulation:draw()

	self:detach(entity, testSprite)
	log.assert(entity.spriteId == nil)
	log.assert(entity.tags.sprite == nil)
end

return Sprite
