local Sprite = require("engine/sprite")
local Template = require("engine/template")

local Wall = {}
Wall.SYSTEM_NAME = "wall"
function Wall:onSimulationCreate(simulation)
	self.simulation = simulation
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.spriteSys:addSprite("wallMetal", 0, 40, 8, 8)
	self.spriteSys:addSprite("wallMetalVerticalLeft", 8, 40, 8, 8)
	self.spriteSys:addSprite("wallMetalVerticalMid", 16, 40, 8, 8)
	self.spriteSys:addSprite("wallMetalVerticalRight", 24, 40, 8, 8)
	self.spriteSys:addSprite("wallMetalHorizontalLeft", 0, 48, 8, 8)
	self.spriteSys:addSprite("wallMetalHorizontalMid", 0, 56, 8, 8)
	self.spriteSys:addSprite("wallMetalHorizontalRight", 0, 64, 8, 8)
	self.spriteSys:addSprite("wallMetalBackground", 32, 40, 8, 8)
	self.template = self.templateSys:add("wall", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "wallMetalBackground",
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
		},
		["editor"] = {
			["category"] = "wall",
		},
	})
end
function Wall:onRunTests()
	self.templateSys:instantiate(self.template)
end

return Wall
