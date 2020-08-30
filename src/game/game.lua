local util = require("src/engine/util")
local client = require("src/engine/client")
local Simulation = require("src/engine/simulation")

local Game = {}
Game.DUMP_FILE = ".\\game_dump.sav"
Game.SAVE_FILE = ".\\game_save.sav"
function Game:populateTestWorld()
	util.log("Game:populateTestWorld()")

	local entitySys = self.simulation:addSystem(require("src/engine/entity"))
	local templateSys = self.simulation:addSystem(require("src/engine/template"))
	local spriteSys = self.simulation:addSystem(require("src/engine/sprite"))
	local textSys = self.simulation:addSystem(require("src/engine/text"))

	local playerTemplate = templateSys:get("player")
	local wallTemplate = templateSys:get("wall")

	-- print(util.toComparable(self.simulation))
	-- print(util.toComparable(playerTemplate))
	local player = templateSys:instantiate(playerTemplate, 120, -32)

	local font = textSys:addFont("test", 0, 192, 8, 8, " ", "_", 8)
	textSys:attach(player, font, "hello, world!")
	player.textTranslationY = -8
	player.textTranslationX = - (4 * #"hello, world!")
	player.textZ = -1

	-- step floors
	for i = 1, 7 do
		local x = -48 + ((i - 1) * 32)
		local y = -48 + ((i - 1) * 32)
		spriteSys:attach(templateSys:instantiate(wallTemplate, x, y, 8, 8),
						 spriteSys:get("wallMetalVerticalLeft"))
		spriteSys:attach(templateSys:instantiate(wallTemplate, x + 8, y, 8, 8),
						 spriteSys:get("wallMetalVerticalMid"))
		spriteSys:attach(templateSys:instantiate(wallTemplate, x + 16, y, 8, 8),
						 spriteSys:get("wallMetalVerticalMid"))
		spriteSys:attach(templateSys:instantiate(wallTemplate, x + 24, y, 8, 8),
						 spriteSys:get("wallMetalVerticalRight"))
	end

	-- side walls
	local levelWidth = 160
	local levelHeight = 120
	templateSys:instantiate(wallTemplate, -64, -64, 8, levelHeight + 128)
	templateSys:instantiate(wallTemplate, levelWidth + 56, -64, 8, levelHeight + 128)

	templateSys:instantiate(wallTemplate, -64, -64, levelWidth + 128, 8)
	templateSys:instantiate(wallTemplate, -64, levelHeight + 56, levelWidth + 128, 8)

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
	util.log("Game:populateTestWorld(): physicsObjectCount=%d", #entitySys:findAll("physicsObject"))
end
function Game:run()
	self.simulation:runTests()

	client.step(client.state)
	self.simulation:addSystem(require("src/game/entities/wall"))
	self.simulation:addSystem(require("src/game/entities/player"))

	self.simulation:create()
	self:populateTestWorld()

	while client.state.running do
		client.step(client.state)
		self.simulation:step()
		self.simulation:draw()

		if client.state.inputX then
			self.simulation:create()
			self:populateTestWorld()
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
