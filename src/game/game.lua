local EngineSys = require("src/engine/engine")
local UtilSys = EngineSys.components.UtilSys
local ClientSys = EngineSys.components.ClientSys
local SimulationSys = EngineSys.components.SimulationSys
local WorldSys = EngineSys.components.WorldSys
local EntitySys = EngineSys.components.EntitySys
local SpriteSys = EngineSys.components.SpriteSys
local TemplateSys = EngineSys.components.TemplateSys

local MaterialSys = require("src/game/material")
local PhysicsSys = require("src/game/physics")
local WallSys = require("src/game/entities/wall")
local PlayerSys = require("src/game/entities/player")

local GameSys = {}
GameSys.DUMP_FILE = ".\\game_dump.sav"
GameSys.SAVE_FILE = ".\\game_save.sav"
GameSys.components = {
	["MaterialSys"] = MaterialSys,
	["PhysicsSys"] = PhysicsSys,
	["WallSys"] = WallSys,
	["PlayerSys"] = PlayerSys,
}
function GameSys.populateTestWorld()
	UtilSys.log("GameSys.populateTestWorld()")

	TemplateSys.instantiate("player", 120, -32)

	-- step floors
	for i = 1, 3 do
		TemplateSys.instantiate("wall", 24 + ((i - 1) * 32), 24 + ((i - 1) * 32), 48, 8)
	end

	-- side walls
	TemplateSys.instantiate("wall", 0, 0, 8, ClientSys.height)
	TemplateSys.instantiate("wall", ClientSys.width - 8, 0, 8, ClientSys.height)

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
	for _ = 1, 15000 do
		local physicsObject = TemplateSys.instantiate("physicsObject")
		EntitySys.setBounds(physicsObject, 8 + math.floor(math.random(140)), math.floor(math.random(64)), 6, 6)
		physicsObject.speedX = math.random(3) - 1.5
		physicsObject.speedY = math.random(3) - 1.5
		-- if EntitySys.findRelative(physicsObject, 0, 0, nil, physicsObject.id) then
		-- 	EntitySys.destroy(physicsObject)
		-- end
	end
	UtilSys.log("GameSys.populateTestWorld(): physicsObjectCount=%d", #EntitySys.findAll("physicsObject"))
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
	jit.on()
	require("jit.opt").start(3)
	GameSys.runTests()

	SimulationSys.create()
	GameSys.populateTestWorld()

	while SimulationSys.isRunning() do
		SimulationSys.step()

		if SimulationSys.inputs.restart then
			WorldSys.create()
			GameSys.populateTestWorld()
		end
	end

	-- SimulationSys.dump(GameSys.DUMP_FILE)
	-- SimulationSys.save(GameSys.SAVE_FILE)
	SimulationSys.destroy()
end

return GameSys
