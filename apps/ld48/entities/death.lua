local util = require("engine/util/util")
local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")

local Death = {}
Death.SYSTEM_NAME = "death"
function Death:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	local deathNames = {"death", "spikeDown", "spikeLeft", "spikeUp", "spikeRight", "spikeAll"}
	local deathParams = {
		["defaults"] = {["w"] = 8, ["h"] = 8, ["offsetX"] = 0, ["offsetY"] = 0, ["selectible"] = true},
		["death"] = {["selectible"] = false},
		["spikeDown"] = {["h"] = 3, ["offsetX"] = 0, ["offsetY"] = 5},
		["spikeLeft"] = {["w"] = 3, },
		["spikeUp"] = {["h"] = 3},
		["spikeRight"] = {["w"] = 3, ["offsetX"] = 5, ["offsetY"] = 0},
	}

	local baseU = 0
	local baseV = 40
	for i, deathName in ipairs(deathNames) do
		local death = util.tableExtend({}, deathParams.defaults, deathParams[deathName] or {})
		local u = baseU + ((i - 1) * 8) + death.offsetX
		self.spriteSys:addSprite(deathName, u, baseV + death.offsetY, death.w, death.h)

		self.templateSys:add(deathName, {
			["properties"] = {
				["w"] = death.w,
				["h"] = death.h,
				["spriteId"] = deathName,
				["offsetX"] = death.offsetX,
				["offsetY"] = death.offsetY,
			},
			["tags"] = {
				["sprite"] = true,
				["material"] = true,
				["air"] = true,
				["death"] = true,
			},
			["editor"] = {
				["category"] = "death",
				["selectible"] = death.selectible,
			},
		})
	end
end

return Death
