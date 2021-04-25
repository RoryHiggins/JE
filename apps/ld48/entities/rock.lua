local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Rock = {}
Rock.SYSTEM_NAME = "rock"
function Rock:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("rock", 0, 8, 6, 6)

	self.templateSys:add("rock", {
		["properties"] = {
			["w"] = 6,
			["h"] = 6,
			["physicsCanPush"] = true,
			["physicsCanCarry"] = true,
			["spriteId"] = "rock",
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["physicsPushable"] = true,
			["physicsCarryable"] = true,
			["physicsObject"] = true,
			["physicsGravityMultiplier"] = 0.25,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})
end

return Rock
