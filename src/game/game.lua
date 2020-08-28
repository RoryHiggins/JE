local EngineSys = require("src/engine/engine")
local UtilSys = require("src/engine/util")
local ClientSys = require("src/engine/client")
local SimulationSys = require("src/engine/simulation")

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
		TemplateSys.instantiate("wall", -48 + ((i - 1) * 32), -48 + ((i - 1) * 32), 48, 8)
	end

	-- side walls
	TemplateSys.instantiate("wall", -64, -64, 8, ClientSys.height + 128)
	TemplateSys.instantiate("wall", ClientSys.width + 56, -64, 8, ClientSys.height + 128)

	TemplateSys.instantiate("wall", -64, -64, ClientSys.width + 128, 8)
	TemplateSys.instantiate("wall", -64, ClientSys.height + 56, ClientSys.width + 128, 8)

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
	for _ = 1, 500 do
		local physicsObject = TemplateSys.instantiate("physicsObject")
		EntitySys.setBounds(physicsObject,
			-48 + math.floor(math.random(ClientSys.width + 96)),
			-48 + math.floor(math.random(ClientSys.height + 96)), 6, 6)
		physicsObject.speedX = math.random(3) - 1.5
		physicsObject.speedY = math.random(3) - 1.5
		if EntitySys.findRelative(physicsObject, 0, 0, "solid") or EntitySys.findRelative(physicsObject, 0, 0, "player") then
			EntitySys.destroy(physicsObject)
		end
	end
	UtilSys.log("GameSys.populateTestWorld(): physicsObjectCount=%d", #EntitySys.findAll("physicsObject"))

	-- Rotating gravity
	-- local steps = 0
	-- table.insert(SimulationSys.stepEvents, function()
	-- 	steps = steps + 1.5
	-- 	local dir = math.rad((math.floor(steps / 90) * 90) % 360)
	-- 	SimulationSys.static.gravityX = math.cos(dir) * 0.8
	-- 	SimulationSys.static.gravityY = math.sin(dir) * 0.8
	-- 	if math.abs(SimulationSys.static.gravityX) < 0.1 then
	-- 		SimulationSys.static.gravityX = 0
	-- 	end
	-- 	if math.abs(SimulationSys.static.gravityY) < 0.1 then
	-- 		SimulationSys.static.gravityY = 0
	-- 	end
	-- end)
end
function GameSys.isRunning()
	return SimulationSys.isRunning()
end
function GameSys.destroy()
	-- SimulationSys.dump(GameSys.DUMP_FILE)
	-- SimulationSys.save(GameSys.SAVE_FILE)
	SimulationSys.destroy()
end
function GameSys.create()
	GameSys.destroy()

	SimulationSys.create()
	GameSys.populateTestWorld()
end
function GameSys.step()
	ClientSys.step()
	SimulationSys.step()

	if SimulationSys.inputs.x then
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
