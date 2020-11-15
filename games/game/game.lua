local util = require("engine/util")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")
local Template = require("engine/systems/template")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")

local Wall = require("games/game/entities/wall")
local Player = require("games/game/entities/player")

local Game = {}
Game.SYSTEM_NAME = "game"
function Game:testWorldInit()
	util.info("")

	local levelW = 256
	local levelH = 256

	self.templateSys:instantiate(self.playerSys.template, 64, 16)

	-- step floors
	for i = 1, 7 do
		local x = 16 + ((i - 1) * 32)
		local y = 16 + ((i - 1) * 32)
		self.spriteSys:attach(self.templateSys:instantiate(self.wallSys.template, x, y, 8, 8),
							  self.spriteSys:get("wallMetalVerticalLeft"))
		self.spriteSys:attach(self.templateSys:instantiate(self.wallSys.template, x + 8, y, 8, 8),
							  self.spriteSys:get("wallMetalVerticalMid"))
		self.spriteSys:attach(self.templateSys:instantiate(self.wallSys.template, x + 16, y, 8, 8),
							  self.spriteSys:get("wallMetalVerticalMid"))
		self.spriteSys:attach(self.templateSys:instantiate(self.wallSys.template, x + 24, y, 8, 8),
							  self.spriteSys:get("wallMetalVerticalRight"))
	end

	-- side walls
	self.templateSys:instantiate(self.wallSys.template, 0, 0, 8, levelH)
	self.templateSys:instantiate(self.wallSys.template, levelW - 8, 0, 8, levelH)

	-- top and bottom walls
	self.templateSys:instantiate(self.wallSys.template, 0, 0, levelW, 8)
	self.templateSys:instantiate(self.wallSys.template, 0, levelH - 8, levelW, 8)

	self.spriteSys:addSprite("physicsObject", 0 + 1, 8 + 2, 6, 6)

	local physicsObjectTemplate = self.templateSys:add("physicsObject", {
		["properties"] = {
			["spriteId"] = "physicsObject",
			["w"] = 6,
			["h"] = 6,
			["physicsCanPush"] = true,
			["physicsCanCarry"] = true,
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["physicsPushable"] = true,
			["physicsCarryable"] = true,
			["physicsObject"] = true,
		},
	})
	for _ = 1, 10 do
		local physicsObject = self.templateSys:instantiate(physicsObjectTemplate)
		self.entitySys:setPos(
			physicsObject,
			16 + math.floor(math.random(levelW - 16)),
			16 + math.floor(math.random(levelH - 16))
		)
		if self.entitySys:findRelative(physicsObject, 0, 0, "solid") then
			self.entitySys:destroy(physicsObject)
		end
	end

	local spriteObjectTemplate = self.templateSys:add("spriteObject", {
		["properties"] = {
			["spriteId"] = "physicsObject",
			["w"] = 6,
			["h"] = 6,
		},
		["tags"] = {
			["sprite"] = true,
		},
	})
	for _ = 1, 0 do
		local spriteObject = self.templateSys:instantiate(spriteObjectTemplate)
		self.entitySys:setPos(
			spriteObject,
			16 + math.floor(math.random(levelW - 16)),
			16 + math.floor(math.random(levelH - 16)))
	end
	util.debug("spriteObjectCount=%d", #self.entitySys:findAll("spriteObject"))
end
function Game:onInit(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)

	self.wallSys = self.simulation:addSystem(Wall)
	self.playerSys = self.simulation:addSystem(Player)

	self.font = self.textSys:addFont("test", 0, 160, 8, 8, " ", "~", 8)
end
function Game:onStart()
	self:testWorldInit()
end
function Game:onStep()
	if self.simulation.started then
		if self.inputSys:getReleased("x") then
			self.simulation:worldInit()
			self:testWorldInit()
		end
	end
end
function Game:onDraw()
	local fps = {
		["x"] = 0,
		["y"] = 0,
		["z"] = -1,
		["text"] = "fps="..tostring(self.simulation.fps)
	}
	self.textSys:draw(fps, self.font, self.simulation.screen)

	--[[local testTriangle = {
		["x1"] = 16,
		["y1"] = 16,
		["x2"] = 24,
		["y2"] = 16,
		["x3"] = 16,
		["y3"] = 32,
		["z"] = -2,

		["r"] = 0,
		["g"] = 1,
		["b"] = 0,
		["a"] = 1,
	}
	self.shapeSys:drawTriangle(testTriangle, self.simulation.screen)

	local testLine = {
		["x"] = 8,
		["y"] = 8,
		["z"] = -3,
		["w"] = -8,
		["h"] = 8,

		["r"] = 1,
		["g"] = 0,
		["b"] = 0,
		["a"] = 1,
	}
	self.shapeSys:drawLine(testLine, self.simulation.screen)

	local testPoint = {
		["x"] = 8,
		["y"] = 16,
		["z"] = -4,

		["r"] = 0,
		["g"] = 0,
		["b"] = 1,
		["a"] = 1,
	}
	self.shapeSys:drawPoint(testPoint, self.simulation.screen)--]]
end

return Game
