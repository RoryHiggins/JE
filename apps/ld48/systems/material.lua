local log = require("engine/util/log")
local Entity = require("engine/systems/entity")

local Material = {}
Material.SYSTEM_NAME = "material"
function Material:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.constants.materials = {
		"air",
		"solid",
		"death",
	}
end
function Material:onEntityTag(entity, tag, tagId)
	if tagId ~= nil then
		local materials = self.simulation.constants.materials
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
	log.assert(self.simulation.constants.materials ~= nil)
end

return Material
