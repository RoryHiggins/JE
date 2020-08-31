local util = require("src/engine/util")
local Simulation = require("src/engine/simulation")

local Game = {}
Game.DUMP_FILE = ".\\game_dump.sav"
Game.SAVE_FILE = ".\\game_save.sav"
function Game:createTestWorld()
	util.log("Game:createTestWorld()")

	local wallSys = self.simulation:addSystem(require("src/game/entities/wall"))
	local playerSys = self.simulation:addSystem(require("src/game/entities/player"))

	local worldSys = self.simulation:addSystem(require("src/engine/world"))
	local entitySys = self.simulation:addSystem(require("src/engine/entity"))
	local templateSys = self.simulation:addSystem(require("src/engine/template"))
	local spriteSys = self.simulation:addSystem(require("src/engine/sprite"))
	local textSys = self.simulation:addSystem(require("src/engine/text"))

	worldSys:create()

	local player = templateSys:instantiate(playerSys.template, 120, -32)

	-- text
	local font = textSys:addFont("test", 0, 192, 8, 8, " ", "_", 8)
	textSys:attach(player, font, "hello, world!")
	player.textTranslationY = -2
	player.textTranslationX = - (0.25 * #"hello, world!")
	player.textZ = -1
	player.textScaleX = 0.125
	player.textScaleY = 0.125

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
		if (entitySys:findRelative(physicsObject, 0, 0, "solid")
			or entitySys:findRelative(physicsObject, 0, 0, "player")) then
			entitySys:destroy(physicsObject)
		end
	end
	util.log("Game:createTestWorld(): physicsObjectCount=%d", #entitySys:findAll("physicsObject"))
end
function Game:run()
	local logLevel = util.logLevel
	util.logLevel = util.LOG_LEVEL_ERR
	self.simulation:runTests()
	util.logLevel = logLevel

	self.simulation:create()
	self:createTestWorld()

	while self.simulation:isRunning() do
		self.simulation:step()
		self.simulation:draw()

		if self.simulation.inputs.x.down then
			self:createTestWorld()
		end
	end

	-- self.simulation:dump(self.DUMP_FILE)
	-- self.simulation:save(self.SAVE_FILE)
	self.simulation:destroy()
end
function Game.new()
	local game = {
		["created"] = false,
		["simulation"] = Simulation.new(),
	}

	Game.__index = Game
	setmetatable(game, Game)

	return game
end

return Game
