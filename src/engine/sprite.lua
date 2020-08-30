local util = require("src/engine/util")
local client = require("src/engine/client")
local Simulation = require("src/engine/simulation")
local Entity = require("src/engine/entity")
local Screen = require("src/engine/screen")


local Sprite = Simulation.createSystem("sprite")
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
			["r"] = r or 255,
			["g"] = g or 255,
			["b"] = b or 255,
			["a"] = a or 255,
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
	local spriteId = sprite.spriteId

	entity.spriteId = spriteId
	self.entitySys:tag(entity, "sprite")
end
function Sprite:detach(entity)
	self.entitySys:untag(entity, "sprite")
	entity.spriteId = nil
end
function Sprite:onSimulationCreate()
	self.entitySys = self.simulation:addSystem(Entity)
	self.screenSys = self.simulation:addSystem(Screen)

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
