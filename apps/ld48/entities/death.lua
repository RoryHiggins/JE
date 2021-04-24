local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Wall = {}
Wall.SYSTEM_NAME = "wall"
function Wall:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("death", 0, 8, 8, 8)

	for _, deathName in ipairs({"death"}) do
		self.templateSys:add(deathName, {
			["properties"] = {
				["w"] = 8,
				["h"] = 8,
				["spriteId"] = deathName,
			},
			["tags"] = {
				["sprite"] = true,
				["material"] = true,
				["death"] = true,
			},
			["editor"] = {
				["category"] = "death",
				["selectible"] = true,
			},
		})
	end
end

return Wall
