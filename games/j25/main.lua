local Simulation = require("engine/simulation")
local Game = require("games/j25/game")

local function main()
	local simulation = Simulation.new()
	simulation:addSystem(Game)
	simulation:run()
end

main()
