local util = require("engine/util/util")
local Simulation = require("engine/systems/simulation")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")
local Editor = require("engine/systems/editor")

local ld48 = {}
ld48.SYSTEM_NAME = "ld48"
function ld48:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)
	self.editorSys = self.simulation:addSystem(Editor)

	self.font = self.textSys:addFont("default", 0, 160, 8, 8, " ", "~", 8)

	self.playerTemplate = self.templateSys:add("player", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,

			["spriteId"] = self.spriteSys:addSprite("player", 16, 0, 8, 8).spriteId,
		},
		["tags"] = {
			["sprite"] = true,
			["player"] = true,
		},
		["editor"] = {
			["category"] = "special",
			["selectible"] = true,
		},
	})

	self.blockTemplate = self.templateSys:add("block", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,

			["spriteId"] = self.spriteSys:addSprite("block", 24, 0, 8, 8).spriteId,
		},
		["tags"] = {
			["sprite"] = true,
			["block"] = true,
		},
		["editor"] = {
			["category"] = "wall",
			["selectible"] = true,
		},
	})

end
function ld48:onWorldInit()
	self.editorSys:starEditing()
end
function ld48:onDraw()
	local fps = {
		["x"] = 0,
		["y"] = 0,
		["z"] = -10,
		["a"] = 0.4,
		["text"] = "fps="..tostring(self.simulation.input.fps)
	}
	self.textSys:draw(fps, self.font, self.simulation.input.screen)
end

local simulation = Simulation.new()
simulation:addSystem(ld48)
simulation:run(...)
