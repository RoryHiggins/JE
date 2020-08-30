local System = require("src/engine/system")
local Entity = require("src/engine/entity")

local Material = System.new("material")
function Material:onSimulationCreate()
	self:addDependencies(Entity)

	self.simulation.static.materials = {
		"air",
		"solid",
		"ice",
		"water",
		"death",
	}
end
function Material:onEntityTag(entity, tag, tagId)
	if tagId ~= nil then
		local materials = self.simulation.static.materials
		local materialsCount = #materials
		for i = 1, materialsCount do
			if tag == materials[i] then
				self.entitySys:tag(entity, "material")
				return
			end
		end
	end
end
function Material:onSimulationTests()
	assert(self.simulation.static.materials ~= nil)
end

return Material
