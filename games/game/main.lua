local Simulation = require("engine/simulation")
local Game = require("games/game/game")

local function main(...)
	local simulation = Simulation.new()
	simulation:addSystem(Game)
	simulation:run(...)
end

main(...)
