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

	-- steps
	for i = 1, 3 do
		TemplateSys.instantiate("wall", 24 + ((i - 1) * 32), 24 + ((i - 1) * 32), 48, 8)
	end

	-- side walls
	TemplateSys.instantiate("wall", 0, 0, 8, ClientSys.height)
	TemplateSys.instantiate("wall", ClientSys.width - 8, 0, 8, ClientSys.height)
	TemplateSys.instantiate("wall", 0, ClientSys.height - 8, ClientSys.width, 8)

	SpriteSys.addSprite("physicsObject", 1, 10, 6, 6)
	TemplateSys.add("physicsObject", {
		["spriteId"] = "physicsObject",
		["w"] = 6,
		["h"] = 6,
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["physicsObject"] = true,
		}
	})
	for _ = 1, 100 do
		local physicsObject = TemplateSys.instantiate("physicsObject")
		EntitySys.setBounds(physicsObject, 8 + math.floor(math.random(140)), math.floor(math.random(64)), 6, 6)
		physicsObject.speedX = math.random(3) - 1.5
		physicsObject.speedY = math.random(3) - 1.5
		if EntitySys.findRelative(physicsObject, 0, 0, "solid") then
			EntitySys.destroy(physicsObject)
		end
	end
	UtilSys.log("GameSys.populateTestWorld(): physicsObjectCount=%d", #EntitySys.findAll("physicsObject"))
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

	if SimulationSys.inputs.restart then
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
