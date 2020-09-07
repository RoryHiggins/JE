local Simulation = require("src/engine/simulation")
local Game = require("src/game/game")

local function main()
	local simulation = Simulation.new()
	simulation:addSystem(Game)
	simulation:run()
end

main()
