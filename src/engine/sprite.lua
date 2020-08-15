local ClientSys = require("src/engine/client")
local SimulationSys = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")
local ScreenSys = require("src/engine/screen")

local ClientSysDrawSprite = ClientSys.drawSprite

local static = SimulationSys.static
static.sprites = {}

local SpriteSys = {}
function SpriteSys.addSprite(spriteId, u, v, w, h, r, g, b, a)
	local sprites = SimulationSys.static.sprites
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

	return sprite
end
function SpriteSys.getSprite(spriteId)
	return SimulationSys.static.sprites[spriteId]
end
function SpriteSys.attach(entity, sprite)
	local spriteId = sprite.spriteId

	entity.spriteId = spriteId
	EntitySys.tag(entity, "sprite")
end
function SpriteSys.detach(entity)
	EntitySys.untag(entity, "sprite")
	entity.spriteId = nil
end
function SpriteSys.runTests()
	SimulationSys.create()

	local entity = EntitySys.create()
	local testSprite = SpriteSys.addSprite("test", 40, 0, 8, 8)

	assert(SpriteSys.getSprite("test") == testSprite)

	SpriteSys.attach(entity, testSprite)
	assert(entity.spriteId == "test")
	assert(entity.tags.sprite)

	SpriteSys.detach(entity, testSprite)
	assert(entity.spriteId == nil)
	assert(entity.tags.sprite == nil)
end
table.insert(ScreenSys.drawEvents, function(screen)
	local sprites = SimulationSys.static.sprites

	local entities = EntitySys.findAll("sprite")
	local entitiesCount = #entities
	for i = 1, entitiesCount do
		local entity = entities[i]
		local sprite = sprites[entity.spriteId]
		ClientSysDrawSprite(entity, sprite, screen)
	end
end)

return SpriteSys
