local ClientSys = require("src/engine/client")
local SimulationSys = require("src/engine/simulation")
local WorldSys = require("src/engine/world")
local EntitySys = require("src/engine/entity")

local ScreenSys = {}
ScreenSys.drawEvents = {}
table.insert(WorldSys.createEvents, function()
	SimulationSys.state.screen = {
		["x"] = 0,
		["y"] = 0,
		["w"] = ClientSys.width,
		["h"] = ClientSys.height
	}
end)
table.insert(SimulationSys.drawEvents, function()
	local screen = SimulationSys.state.screen
	local screenTarget = EntitySys.find("screenTarget")
	if screenTarget then
		screen.x = math.floor(screenTarget.x - (screen.w / 2.5))
		screen.y = math.floor(screenTarget.y - (screen.h / 2.5))
	end

	local events = ScreenSys.drawEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](screen)
	end
end)

return ScreenSys