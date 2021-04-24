local util = require("engine/util/util")
local Simulation = require("engine/systems/simulation")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")
local Editor = require("engine/systems/editor")

local MainMenu = require("apps/ld48/systems/main_menu")
local Material = require("apps/ld48/systems/material")
local Physics = require("apps/ld48/systems/physics")
local Wall = require("apps/ld48/entities/wall")
local Player = require("apps/ld48/entities/player")

local ld48 = {}
ld48.SYSTEM_NAME = "ld48"
ld48.modePlay = "play"
ld48.modeEditor = "editor"
ld48.modePlayTestWorld = "playTestWorld"
ld48.modeResume = "resume"
function ld48:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)
	self.editorSys = self.simulation:addSystem(Editor)

	self.mainMenuSys = self.simulation:addSystem(MainMenu)
	self.materialSys = self.simulation:addSystem(Material)
	self.physicsSys = self.simulation:addSystem(Physics)
	self.wallSys = self.simulation:addSystem(Wall)
	self.playerSys = self.simulation:addSystem(Player)

	self.font = self.textSys:getDefaultFont()
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
-- function ld48:onWorldInit()
-- end
function ld48:onStart()
	self.simulation.constants.developerDebugging = true
	self.mode = ld48.modePlay

	-- self.mode = ld48.modeResume
	-- self.mode = ld48.modeEditor
	-- self.mode = ld48.modePlayTestWorld

	if self.mode == ld48.modePlay then
		self.mainMenuSys:start()
	end
	if self.mode == ld48.modeEditor then
		self.simulation.constants.developerDebugging = true
		self.editorSys:startEditor()
	end
	if self.mode == ld48.modePlayTestWorld then
		self.simulation.constants.developerDebugging = true
		self.playerSys:createWorld("test")
		self.simulation:worldInit()
		self:testWorldInit()
	end
	if self.mode == ld48.modeResume then
		self.simulation:load(Simulation.SAVE_FILE)
	end
end
-- function ld48:onDraw()
-- 	self.textSys:drawDebugString("fps="..tostring(self.simulation.input.fps))
-- end

local simulation = Simulation.new()
simulation:addSystem(ld48)
simulation:run(...)
