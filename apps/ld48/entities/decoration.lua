local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Decoration = {}
Decoration.SYSTEM_NAME = "decoration"
function Decoration:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("puddle", 0, 56, 8, 8)
	self.templateSys:add("puddle", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["a"] = 0.25,
			["spriteId"] = "puddle",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	self.spriteSys:addSprite("rubble", 8, 56, 8, 8)
	self.templateSys:add("rubble", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "rubble",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	self.spriteSys:addSprite("mushroom", 16, 56, 8, 8)
	self.templateSys:add("mushroom", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "mushroom",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	self.spriteSys:addSprite("stalagtite", 24, 56, 8, 8)
	self.templateSys:add("stalagtite", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "stalagtite",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	self.spriteSys:addSprite("moss1", 32, 56, 8, 8)
	self.templateSys:add("moss1", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "moss1",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	self.spriteSys:addSprite("moss2", 40, 56, 8, 8)
	self.templateSys:add("moss2", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "moss2",
		},
		["tags"] = {
			["sprite"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})
end

return Decoration
