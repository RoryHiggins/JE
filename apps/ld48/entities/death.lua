local client = require("engine/client/client")
local util = require("engine/util/util")
local Sprite = require("engine/systems/sprite")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")

local Death = {}
Death.SYSTEM_NAME = "death"
function Death:onInit(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)
	self.entitySys = self.simulation:addSystem(Entity)

	local deathNames = {
		"death",
		"spikeDown",
		"spikeLeft",
		"spikeUp",
		"spikeRight",
		"spikeAll",
		"spikeStoneDown",
		"spikeStoneLeft",
		"spikeStoneUp",
		"spikeStoneRight",
		"lava1",
		"lava2",
		"lava3",
		"lava4",
		"spikeHellDown",
		"spikeHellLeft",
		"spikeHellUp",
		"spikeHellRight",
	}
	local deathParams = {
		["defaults"] = {["w"] = 8, ["h"] = 8, ["offsetX"] = 0, ["offsetY"] = 0, ["selectible"] = true, ["makeTemplate"] = true},
		["death"] = {["selectible"] = false},
		["spikeDown"] = {["h"] = 3, ["offsetY"] = 5},
		["spikeLeft"] = {["w"] = 3},
		["spikeUp"] = {["h"] = 3},
		["spikeRight"] = {["w"] = 3, ["offsetX"] = 5},

		["spikeStoneDown"] = {["h"] = 5, ["offsetY"] = 3},
		["spikeStoneLeft"] = {["w"] = 5},
		["spikeStoneUp"] = {["h"] = 5},
		["spikeStoneRight"] = {["w"] = 5, ["offsetX"] = 3},

		["lava1"] = {["makeTemplate"] = false},
		["lava2"] = {["makeTemplate"] = false},
		["lava3"] = {["makeTemplate"] = false},
		["lava4"] = {["makeTemplate"] = false},

		["spikeHellDown"] = {["h"] = 5, ["offsetY"] = 3},
		["spikeHellLeft"] = {["w"] = 5},
		["spikeHellUp"] = {["h"] = 5},
		["spikeHellRight"] = {["w"] = 5, ["offsetX"] = 3},

	}

	local baseU = 0
	local baseV = 40
	for i, deathName in ipairs(deathNames) do
		local death = util.tableExtend({}, deathParams.defaults, deathParams[deathName] or {})
		local u = baseU + ((i - 1) * 8) + death.offsetX
		self.spriteSys:addSprite(deathName, u, baseV + death.offsetY, death.w, death.h)

		if death.makeTemplate then
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

	self.templateSys:add("lava", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "lava1",
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["air"] = true,
			["death"] = true,
			["lava"] = true,
		},
		["editor"] = {
			["category"] = "death",
			["selectible"] = true,
		},
	})
end
function Death:onStep()
	local lavaAnimationIndex = 1 + math.floor(client.state.frame / 20) % 4
	for _, lava in ipairs(self.entitySys:findAll("lava")) do
		lava.spriteId = "lava"..lavaAnimationIndex
	end
end

return Death
