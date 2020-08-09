local SimulationSys = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")

SimulationSys.static.templates = {}

local TemplateSys = {}
function TemplateSys.add(templateId, template)
	local templates = SimulationSys.static.templates
	local currentTemplate = templates[templateId]
	if currentTemplate == nil then
		templates[templateId] = template
	end
	return template
end
function TemplateSys.instantiate(templateId, x, y, w, h)
	local entity = EntitySys.create(SimulationSys.static.templates[templateId])

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

	EntitySys.setBounds(entity, x, y, w, h)

	return entity

end
function TemplateSys.runTests()
	SimulationSys.create()
	assert(SimulationSys.static.templates ~= nil)

	local template = {["x"] = 2, ["tags"] = {["yee"] = true}}
	TemplateSys.add("yee", template)
	EntitySys.create(SimulationSys.static.templates["yee"])
	assert(EntitySys.find("yee").x == 2)
	assert(EntitySys.find("yee").tags["yee"] ~= nil)
end


return TemplateSys
