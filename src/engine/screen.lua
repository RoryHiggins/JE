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
		screen.x = screenTarget.x - math.floor(ClientSys.width / 2)
		screen.y = screenTarget.y - math.floor(ClientSys.height / 2)
	end

	local events = ScreenSys.drawEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](screen)
	end
end)

return ScreenSys