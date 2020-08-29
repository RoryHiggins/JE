local SimulationSys = require("src/engine/simulation")
local WorldSys = require("src/engine/world")
local EntitySys = require("src/engine/entity")

local ScreenSys = {}
ScreenSys.drawEvents = {}
table.insert(WorldSys.createEvents, function()
	SimulationSys.state.screen = {
		["x"] = 0,
		["y"] = 0,
	}
end)
table.insert(SimulationSys.drawEvents, function()
	local screen = SimulationSys.state.screen
	local screenTarget = EntitySys.find("screenTarget")
	if screenTarget then
		screen.x = screenTarget.x
		screen.y = screenTarget.y
	end

	local events = ScreenSys.drawEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](screen)
	end
end)

return ScreenSys