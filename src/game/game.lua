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
	local textSys = self.simulation:addSystem(require("src/engine/text"))

	local player = templateSys:instantiate(playerSys.template, 120, -32)

	-- text
	local font = textSys:addFont("test", 0, 192, 8, 8, " ", "_", 8)

	textSys:attach(player, font, "!!!")
	player.textZ = -1
	player.textTranslationX = 8

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
	local levelWidth = 160
	local levelHeight = 120
	templateSys:instantiate(wallSys.template, -64, -64, 8, levelHeight + 128)
	templateSys:instantiate(wallSys.template, levelWidth + 56, -64, 8, levelHeight + 128)

	-- top and bottom walls
	templateSys:instantiate(wallSys.template, -64, -64, levelWidth + 128, 8)
	templateSys:instantiate(wallSys.template, -64, levelHeight + 56, levelWidth + 128, 8)

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
			-48 + math.floor(math.random(levelWidth + 96)),
			-48 + math.floor(math.random(levelHeight + 96)), 6, 6)
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

	self:createTestWorld()
end
function Game:onSimulationStep()
	if self.simulation.inputs.x.released then
		self:createTestWorld()
	end
end

return Game
