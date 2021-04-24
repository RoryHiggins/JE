local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Wall = {}
Wall.SYSTEM_NAME = "wall"
function Wall:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("wallBlack", 0, 24, 8, 8)
	self.spriteSys:addSprite("wallRock", 8, 24, 8, 8)

	for _, wallName in ipairs({"wallBlack", "wallRock"}) do
		self.templateSys:add(wallName, {
			["properties"] = {
				["w"] = 8,
				["h"] = 8,
				["spriteId"] = wallName,
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

end

return Wall
