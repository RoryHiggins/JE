local Simulation = require("engine/systems/simulation")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Sprite = require("engine/systems/sprite")

local MinimalGame = {}
MinimalGame.SYSTEM_NAME = "minimalGame"
function MinimalGame:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.spriteSys = self.simulation:addSystem(Sprite)

	-- create a template we'll use for creating entities in the world
	self.blockTemplate = self.templateSys:add("block", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,

			["r"] = 0,
			["g"] = 0,
			["b"] = 0,
			["a"] = 1,

			["spriteId"] = self.spriteSys:getUntextured().spriteId,
		},
		["tags"] = {
			["sprite"] = true,
			["block"] = true,  -- used in OnStep to find block instances by tag
		},
	})
end
function MinimalGame:onWorldInit()
	-- create two instances of the template with different colors and positions
	local redBlock = self.templateSys:instantiate(self.blockTemplate, 32, 32)
	redBlock.r = 1
	redBlock.g = 0
	redBlock.b = 0

	local blueBlock = self.templateSys:instantiate(self.blockTemplate, 48, 56)
	blueBlock.r = 0
	blueBlock.g = 0
	blueBlock.b = 1
end
function MinimalGame:onStep()
	-- move blocks in random directions
	for _, block in pairs(self.entitySys:findAll("block")) do
		self.entitySys:movePos(block, math.floor(math.random(3) - 2), math.floor(math.random(3) - 2))
	end
end

local simulation = Simulation.new()
simulation:addSystem(MinimalGame)
simulation:run(...)
