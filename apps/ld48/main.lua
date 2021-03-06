local util = require("engine/util/util")
local Simulation = require("engine/systems/simulation")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Audio = require("engine/systems/audio")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")

local Editor = require("apps/ld48/systems/editor")
local MainMenu = require("apps/ld48/systems/main_menu")
local Material = require("apps/ld48/systems/material")
local Physics = require("apps/ld48/systems/physics")
local Wall = require("apps/ld48/entities/wall")
local Death = require("apps/ld48/entities/death")
local Player = require("apps/ld48/entities/player")

local ld48 = {}
ld48.SYSTEM_NAME = "ld48"
ld48.modePlay = "play"
ld48.modeEditor = "editor"
ld48.modePlayInEditor = "playInEditor"
function ld48:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.audioSys = self.simulation:addSystem(Audio)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)
	self.editorSys = self.simulation:addSystem(Editor)

	self.mainMenuSys = self.simulation:addSystem(MainMenu)
	self.materialSys = self.simulation:addSystem(Material)
	self.physicsSys = self.simulation:addSystem(Physics)
	self.wallSys = self.simulation:addSystem(Wall)
	self.deathSys = self.simulation:addSystem(Death)
	self.rockSys = self.simulation:addSystem(require("apps/ld48/entities/rock"))
	self.decorationSys = self.simulation:addSystem(require("apps/ld48/entities/decoration"))
	self.playerSys = self.simulation:addSystem(Player)

	self.font = self.textSys:getDefaultFont()
end
function ld48:onStart()
	self.mode = ld48.modePlay
	self.simulation.constants.developerDebugging = false
	local editorWorld = "hell1"

	if self.mode == ld48.modePlay then
		self.mainMenuSys:startMainMenu()
	end
	if self.mode == ld48.modeEditor then
		self.editorSys:startEditor(editorWorld)
	end
	if self.mode == ld48.modePlayInEditor then
		self.editorSys:startEditor(editorWorld)
		self.editorSys:setMode(self.editorSys.modePlaying)
	end

	self.audioSys:stopAllAudio()
	self.audioSys:playAudio("apps/ld48/data/song1.wav", true)
end
function ld48:onDraw()
	self.textSys:drawDebugString("fps="..tostring(self.simulation.input.fps))
end

local simulation = Simulation.new()
simulation:addSystem(ld48)
simulation:run(...)
