local Simulation = require("engine/systems/simulation")
local Game = require("apps/example_platformer/game")

local function main(...)
	local simulation = Simulation.new()
	simulation:addSystem(Game)
	simulation:run(...)
end

main(...)
