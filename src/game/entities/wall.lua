local Simulation = require("src/engine/simulation")
local Sprite = require("src/engine/sprite")
local Template = require("src/engine/template")

local Wall = Simulation.createSystem("wall")
function Wall:onSimulationCreate()
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
		["w"] = 8,
		["h"] = 8,
		["spriteId"] = "wallMetalBackground",
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
		}
	})
end
function Wall:onSimulationTests()
	self.templateSys:instantiate(self.template)
	for _ = 1, 10 do
		self.simulation:step()
	end
end

return Wall
