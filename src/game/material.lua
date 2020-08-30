local simulation = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")

local MaterialSys = {}
simulation.static.materials = {
	"air",
	"solid",
	"ice",
	"water",
	"death",
}
table.insert(EntitySys.tagEvents, function(entity, tag, tagId)
	if tagId ~= nil then
		local materials = simulation.static.materials
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
