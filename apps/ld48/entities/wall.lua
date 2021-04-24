local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Wall = {}
Wall.SYSTEM_NAME = "wall"
function Wall:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("wall", 24, 0, 8, 8)
	self.template = self.templateSys:add("wall", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "wall",
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
		},
		["editor"] = {
			["category"] = "wall",
			["selectible"] = true,
		},
	})
end

return Wall
