local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Wall = {}
Wall.SYSTEM_NAME = "wall"
function Wall:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	local wallNames = {"wallRock", "wallMarble"}

	local tags = {
		["sprite"] = true,
		["material"] = true,
		["solid"] = true,
	}

	local baseU = 0
	local baseV = 24
	self.spriteSys:addSprite("wallBlack", baseU, baseV, 8, 8)
	for _, size in ipairs({8, 16, 32}) do
		local deathName = "wallBlack"
		if size > 8 then
			deathName = deathName..size
		end

		self.templateSys:add(deathName, {
			["properties"] = {
				["w"] = size,
				["h"] = size,
				["spriteId"] = "wallBlack",
			},
			["tags"] = tags,
			["editor"] = {
				["category"] = "wall",
				["selectible"] = true,
				["overlappable"] = true,
			},
		})
	end

	baseU = 8
	for i, wallName in ipairs(wallNames) do
		local u = baseU + ((i - 1) * 8)
		self.spriteSys:addSprite(wallName, u, baseV, 8, 8)

		self.templateSys:add(wallName, {
			["properties"] = {
				["w"] = 8,
				["h"] = 8,
				["spriteId"] = wallName,
			},
			["tags"] = tags,
			["editor"] = {
				["category"] = "wall",
				["selectible"] = true,
			},
		})
	end

end

return Wall
