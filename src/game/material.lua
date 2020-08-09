local EngineSys = require("src/engine/engine")
local SimulationSys = EngineSys.components.SimulationSys
local EntitySys = EngineSys.components.EntitySys

local MaterialSys = {}
SimulationSys.static.materials = {
	"air",
	"solid",
	"ice",
	"water",
	"death",
}
table.insert(EntitySys.tagEvents, function(entity, tag, tagId)
	if tagId ~= nil then
		local materials = SimulationSys.static.materials
		local materialsCount = #materials
		for i = 1, materialsCount do
			if tag == materials[i] then
				EntitySys.tag(entity, "material")
				return
			end
		end
	end
end)

return MaterialSys
