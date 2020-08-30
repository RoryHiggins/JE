local EngineSys = require("src/engine/engine")
local UtilSys = require("src/engine/util")
local client = require("src/engine/client")
local simulation = require("src/engine/simulation")

local GameSys = {}
GameSys.DUMP_FILE = ".\\game_dump.sav"
GameSys.SAVE_FILE = ".\\game_save.sav"
GameSys.components = {
	["MaterialSys"] = require("src/game/material"),
	["PhysicsSys"] = require("src/game/physics"),
	["WallSys"] = require("src/game/entities/wall"),
	["PlayerSys"] = require("src/game/entities/player"),
}
function GameSys.populateTestWorld()
	UtilSys.log("GameSys.populateTestWorld()")

	local EntitySys = require("src/engine/entity")
	local TemplateSys = require("src/engine/template")
	local SpriteSys = require("src/engine/sprite")

	TemplateSys.instantiate("player", 120, -32)

	-- step floors
	for i = 1, 7 do
		local x = -48 + ((i - 1) * 32)
		local y = -48 + ((i - 1) * 32)
		SpriteSys.attach(TemplateSys.instantiate("wall", x, y, 8, 8), SpriteSys.getSprite("wallMetalVerticalLeft"))
		SpriteSys.attach(TemplateSys.instantiate("wall", x + 8, y, 8, 8), SpriteSys.getSprite("wallMetalVerticalMid"))
		SpriteSys.attach(TemplateSys.instantiate("wall", x + 16, y, 8, 8), SpriteSys.getSprite("wallMetalVerticalMid"))
		SpriteSys.attach(TemplateSys.instantiate("wall", x + 24, y, 8, 8), SpriteSys.getSprite("wallMetalVerticalRight"))
	end

	-- side walls
	local levelWidth = 160
	local levelHeight = 120
	TemplateSys.instantiate("wall", -64, -64, 8, levelHeight + 128)
	TemplateSys.instantiate("wall", levelWidth + 56, -64, 8, levelHeight + 128)

	TemplateSys.instantiate("wall", -64, -64, levelWidth + 128, 8)
	TemplateSys.instantiate("wall", -64, levelHeight + 56, levelWidth + 128, 8)

	SpriteSys.addSprite("physicsObject", 1, 10, 6, 6)
	TemplateSys.add("physicsObject", {
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
		local physicsObject = TemplateSys.instantiate("physicsObject")
		EntitySys.setBounds(physicsObject,
			-48 + math.floor(math.random(levelWidth + 96)),
			-48 + math.floor(math.random(levelHeight + 96)), 6, 6)
		physicsObject.z = physicsObject.y
		physicsObject.speedX = math.random(3) - 1.5
		physicsObject.speedY = math.random(3) - 1.5
		if EntitySys.findRelative(physicsObject, 0, 0, "solid") or EntitySys.findRelative(physicsObject, 0, 0, "player") then
			EntitySys.destroy(physicsObject)
		end
	end
	UtilSys.log("GameSys.populateTestWorld(): physicsObjectCount=%d", #EntitySys.findAll("physicsObject"))

	-- Rotating gravity
	-- local steps = 90
	-- table.insert(simulation.stepEvents, function()
	-- 	steps = steps + 0.25
	-- 	local dir = math.rad((math.floor(steps / 90) * 90) % 360)
	-- 	simulation.static.physicsGravityX = math.cos(dir) * 0.5
	-- 	simulation.static.physicsGravityY = math.sin(dir) * 0.5
	-- 	if math.abs(simulation.static.physicsGravityX) < 0.1 then
	-- 		simulation.static.physicsGravityX = 0
	-- 	end
	-- 	if math.abs(simulation.static.physicsGravityY) < 0.1 then
	-- 		simulation.static.physicsGravityY = 0
	-- 	end
	-- end)
end
function GameSys.isRunning()
	return simulation.isRunning()
end
function GameSys.destroy()
	-- simulation.dump(GameSys.DUMP_FILE)
	-- simulation.save(GameSys.SAVE_FILE)
	simulation.destroy()
end
function GameSys.create()
	GameSys.destroy()

	simulation.create()
	GameSys.populateTestWorld()
end
function GameSys.step()
	client.step(client.state)
	simulation.step()

	if client.state.inputX then
		GameSys.create()
	end
end
function GameSys.runTests()
	UtilSys.log("GameSys.runTests(): Running automated tests")

	EngineSys.runTests()

	for componentName, component in pairs(GameSys.components) do
		if component.runTests then
			UtilSys.log("GameSys.runTests(): Running tests for %s", componentName)
			component.runTests()
		else
			UtilSys.log("GameSys.runTests(): No tests for %s", componentName)
		end
	end

	GameSys.create()

	-- simulate a few frames of gameplay
	for _ = 1, 5 do
		GameSys.step()
	end
end
function GameSys.run()
	GameSys.runTests()

	GameSys.create()

	while GameSys.isRunning() do
		GameSys.step()
	end

	GameSys.destroy()
end

return GameSys
