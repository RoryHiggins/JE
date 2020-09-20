local Entity = require("engine/entity")

local Template = {}
Template.SYSTEM_NAME = "template"
function Template:add(templateId, template)
	local templates = self.simulation.static.templates
	local currentTemplate = templates[templateId]
	if currentTemplate == nil then
		templates[templateId] = template
	end
	return template
end
function Template:get(templateId)
	return self.simulation.static.templates[templateId]
end
function Template:instantiate(template, x, y, w, h)
	local entity = self.entitySys:create(template)

	if x == nil then
		x = entity.x
	end
	if y == nil then
		y = entity.y
	end

	if w == nil then
		w = entity.w
	end
	if h == nil then
		h = entity.h
	end

	self.entitySys:setBounds(entity, x, y, w, h)

	return entity

end
function Template:onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.static.templates = {}
end
function Template:onRunTests()
	assert(self.simulation.static.templates ~= nil)

	local template = self:add("yee", {
		["properties"] = {
			["x"] = 2,
		},
		["tags"] = {
			["yee"] = true
		},
	})
	assert(self:get("yee") == template)

	local entity = self:instantiate(template)
	assert(self.entitySys:find("yee") == entity)
	assert(self.entitySys:find("yee").x == 2)
	assert(self.entitySys:find("yee").tags["yee"] ~= nil)
end

return Template
