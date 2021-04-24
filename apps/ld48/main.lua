local util = require("engine/util/util")
local Simulation = require("engine/systems/simulation")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")
local Editor = require("engine/systems/editor")

local Material = require("apps/ld48/systems/material")
local Physics = require("apps/ld48/systems/physics")
local Wall = require("apps/ld48/entities/wall")
local Player = require("apps/ld48/entities/player")

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

	self.materialSys = self.simulation:addSystem(Material)
	self.physicsSys = self.simulation:addSystem(Physics)
	self.wallSys = self.simulation:addSystem(Wall)
	self.playerSys = self.simulation:addSystem(Player)

	self.font = self.textSys:addFont("default", 0, 160, 8, 8, " ", "~", 8)
end
function ld48:testWorldInit()
	self.templateSys:instantiate(self.playerSys.template, 64, 16)

	-- step floors
	for i = 1, 7 do
		local x = 16 + ((i - 1) * 32)
		local y = 16 + ((i - 1) * 32)
		self.templateSys:instantiate(self.wallSys.template, x, y, 8, 8)
		self.templateSys:instantiate(self.wallSys.template, x + 8, y, 8, 8)
		self.templateSys:instantiate(self.wallSys.template, x + 16, y, 8, 8)
		self.templateSys:instantiate(self.wallSys.template, x + 24, y, 8, 8)
	end
end
function ld48:onWorldInit()
	self.editorMode = false

	if self.editorMode then
		self.editorSys:startEditing()
	else
		self:testWorldInit()
	end
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
