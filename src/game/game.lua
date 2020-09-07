local util = require("src/engine/util")

local Game = {}
Game.SYSTEM_NAME = "game"
function Game:createTestWorld()
	util.info("Game:createTestWorld()")

	local wallSys = self.simulation:addSystem(require("src/game/entities/wall"))
	local playerSys = self.simulation:addSystem(require("src/game/entities/player"))

	local worldSys = self.simulation:addSystem(require("src/engine/world"))
	worldSys:create()

	local entitySys = self.simulation:addSystem(require("src/engine/entity"))
	local templateSys = self.simulation:addSystem(require("src/engine/template"))
	local spriteSys = self.simulation:addSystem(require("src/engine/sprite"))

	templateSys:instantiate(playerSys.template, 120, -32)

	-- step floors
	for i = 1, 7 do
		local x = -48 + ((i - 1) * 32)
		local y = -48 + ((i - 1) * 32)
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
	local levelW = self.simulation.screen.w
	local levelH = self.simulation.screen.h
	templateSys:instantiate(wallSys.template, -64, -64, 8, levelH + 128)
	templateSys:instantiate(wallSys.template, levelW + 56, -64, 8, levelH + 128)

	-- top and bottom walls
	templateSys:instantiate(wallSys.template, -64, -64, levelW + 128, 8)
	templateSys:instantiate(wallSys.template, -64, levelH + 56, levelW + 128, 8)

	spriteSys:addSprite("physicsObject", 1, 10, 6, 6)

	local physicsObjectTemplate = templateSys:add("physicsObject", {
		["spriteId"] = "physicsObject",
		["w"] = 6,
		["h"] = 6,
		["physicsCanPush"] = true,
		["physicsCanCarry"] = true,
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["physicsPushable"] = true,
			["physicsCarryable"] = true,
			["physicsObject"] = true,
		}
	})
	for _ = 1, 10 do
		local physicsObject = templateSys:instantiate(physicsObjectTemplate)
		entitySys:setBounds(physicsObject,
			-48 + math.floor(math.random(levelW + 96)),
			-48 + math.floor(math.random(levelH + 96)), 6, 6)
		physicsObject.z = physicsObject.y
		physicsObject.speedX = math.random(3) - 1.5
		physicsObject.speedY = math.random(3) - 1.5
		if entitySys:findRelative(physicsObject, 0, 0, "solid") then
			entitySys:destroy(physicsObject)
		end
	end
	util.debug("Game:createTestWorld(): physicsObjectCount=%d", #entitySys:findAll("physicsObject"))
end
function Game:onSimulationCreate(simulation)
	self.simulation = simulation
	self.textSys = self.simulation:addSystem(require("src/engine/text"))

	self.font = self.textSys:addFont("test", 0, 192, 8, 8, " ", "_", 8)
	self:createTestWorld()
end
function Game:onSimulationStep()
	if self.simulation.inputs.x.released then
		self:createTestWorld()
	end
end
function Game:onSimulationDraw(screen)
	-- TODO "Renderable" class
	local fps = {
		["x"] = 0,
		["y"] = 0,
		["z"] = -10,
		["text"] = tostring(self.simulation.fps)
	}
	self.textSys:draw(fps, self.font, screen)
end

return Game
