local util = require("engine/util")
local Input = require("engine/input")
local Text = require("engine/text")
local Shape = require("engine/shape")

local Game = {}
Game.SYSTEM_NAME = "game"
function Game:createTestWorld()
	util.info("")

	local wallSys = self.simulation:addSystem(require("games/game/entities/wall"))
	local playerSys = self.simulation:addSystem(require("games/game/entities/player"))

	local worldSys = self.simulation:addSystem(require("engine/world"))
	worldSys:create()

	local entitySys = self.simulation:addSystem(require("engine/entity"))
	local templateSys = self.simulation:addSystem(require("engine/template"))
	local spriteSys = self.simulation:addSystem(require("engine/sprite"))

	local levelW = 256
	local levelH = 256

	templateSys:instantiate(playerSys.template, 64, 16)

	-- step floors
	for i = 1, 7 do
		local x = 16 + ((i - 1) * 32)
		local y = 16 + ((i - 1) * 32)
		spriteSys:attach(templateSys:instantiate(wallSys.template, x, y, 8, 8),
						 spriteSys:get("wallMetalVerticalLeft"))
		spriteSys:attach(templateSys:instantiate(wallSys.template, x + 8, y, 8, 8),
						 spriteSys:get("wallMetalVerticalMid"))
		spriteSys:attach(templateSys:instantiate(wallSys.template, x + 16, y, 8, 8),
						 spriteSys:get("wallMetalVerticalMid"))
		spriteSys:attach(templateSys:instantiate(wallSys.template, x + 24, y, 8, 8),
						 spriteSys:get("wallMetalVerticalRight"))
	end

	-- side walls
	templateSys:instantiate(wallSys.template, 0, 0, 8, levelH)
	templateSys:instantiate(wallSys.template, levelW - 8, 0, 8, levelH)

	-- top and bottom walls
	templateSys:instantiate(wallSys.template, 0, 0, levelW, 8)
	templateSys:instantiate(wallSys.template, 0, levelH - 8, levelW, 8)

	spriteSys:addSprite("physicsObject", 0 + 1, 8 + 2, 6, 6)

	local physicsObjectTemplate = templateSys:add("physicsObject", {
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
		local physicsObject = templateSys:instantiate(physicsObjectTemplate)
		entitySys:setPos(
			physicsObject,
			16 + math.floor(math.random(levelW - 16)),
			16 + math.floor(math.random(levelH - 16))
		)
		if entitySys:findRelative(physicsObject, 0, 0, "solid") then
			entitySys:destroy(physicsObject)
		end
	end

	local spriteObjectTemplate = templateSys:add("spriteObject", {
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
		local spriteObject = templateSys:instantiate(spriteObjectTemplate)
		entitySys:setPos(
			spriteObject,
			16 + math.floor(math.random(levelW - 16)),
			16 + math.floor(math.random(levelH - 16)))
	end
	util.debug("spriteObjectCount=%d", #entitySys:findAll("spriteObject"))
end
function Game:onSimulationCreate(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)

	self.font = self.textSys:addFont("test", 0, 160, 8, 8, " ", "~", 8)
end
function Game:onSimulationStart()
	self:createTestWorld()
end
function Game:onSimulationStep()
	if self.simulation.started then
		if self.inputSys:getReleased("x") then
			self:createTestWorld()
		end
	end
end
function Game:onSimulationDraw(screen)
	local fps = {
		["x"] = 0,
		["y"] = 0,
		["z"] = -1,
		["text"] = "fps="..tostring(self.simulation.fps)
	}
	self.textSys:draw(fps, self.font, screen)

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
	self.shapeSys:drawTriangle(testTriangle, screen)

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
	self.shapeSys:drawLine(testLine, screen)

	local testPoint = {
		["x"] = 8,
		["y"] = 16,
		["z"] = -4,

		["r"] = 0,
		["g"] = 0,
		["b"] = 1,
		["a"] = 1,
	}
	self.shapeSys:drawPoint(testPoint, screen)--]]
end

return Game