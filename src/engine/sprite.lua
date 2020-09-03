local util = require("src/engine/util")
local client = require("src/engine/client")
local System = require("src/engine/system")
local Entity = require("src/engine/entity")
local Screen = require("src/engine/screen")


local Sprite = System.new("sprite")
function Sprite:addSprite(spriteId, u, v, w, h, r, g, b, a)
	local sprites = self.simulation.static.sprites
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

	util.debug("Sprite:addSprite(): sprite=%s", util.toComparable(sprite))

	return sprite
end
function Sprite:get(spriteId)
	return self.simulation.static.sprites[spriteId]
end
function Sprite:attach(entity, sprite)
	entity.spriteId = sprite.spriteId
	self.entitySys:tag(entity, "sprite")
end
function Sprite:detach(entity)
	self.entitySys:untag(entity, "sprite")
	entity.spriteId = nil
end
function Sprite:onSimulationCreate()
	self:addDependencies(Entity, Screen)

	self.simulation.static.sprites = {}
end
function Sprite:onScreenDraw(screen)
	local sprites = self.simulation.static.sprites

	for _, entity in ipairs(self.entitySys:findAll("sprite")) do
		client.drawSprite(entity, sprites[entity.spriteId], screen)
	end
end
function Sprite:onSimulationTests()
	local entity = self.entitySys:create()
	local testSprite = self:addSprite("test", 40, 0, 8, 8)

	assert(self:get("test") == testSprite)

	self:attach(entity, testSprite)
	assert(entity.spriteId == "test")
	assert(entity.tags.sprite)

	self:detach(entity, testSprite)
	assert(entity.spriteId == nil)
	assert(entity.tags.sprite == nil)
end

return Sprite
