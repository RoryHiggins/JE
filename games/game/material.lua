local Entity = require("engine/entity")

local Material = {}
Material.SYSTEM_NAME = "material"
function Material:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

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
function Material:onRunTests()
	assert(self.simulation.static.materials ~= nil)
end

return Material
