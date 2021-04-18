local log = require("engine/util/log")
local util = require("engine/util/util")
local Entity = require("engine/systems/entity")

local Template = {}
Template.SYSTEM_NAME = "template"
function Template:add(templateId, template)
	local templates = self.simulation.constants.templates
	local currentTemplate = templates[templateId]
	if currentTemplate == nil then
		templates[templateId] = template
	end
	return template
end
function Template:get(templateId)
	return self.simulation.constants.templates[templateId]
end
function Template:apply(entity, template)
	local entitySys = self.entitySys

	local templateProperties = template.properties
	if templateProperties ~= nil then
		-- backup fields that cannot be set by a template
		local entityId = entity.id
		local x = entity.x
		local y = entity.y
		local w = entity.w
		local h = entity.h

		util.tableExtend(entity, templateProperties)

		-- restore fields that cannot be set by a template
		entity.x = x
		entity.y = y
		entity.w = w
		entity.h = h
		entity.id = entityId

		-- set bounds from template
		x = templateProperties.x or x
		y = templateProperties.y or y
		w = templateProperties.w or w
		h = templateProperties.h or h
		entitySys:setBounds(entity, x, y, w, h)
	end

	local templateTags = template.tags
	if templateTags ~= nil then
		for tag, enabled in pairs(templateTags) do
			if enabled then
				entitySys:tag(entity, tag)
			end
		end
	end
end
function Template:instantiate(template, x, y, w, h)
	local entity = self.entitySys:create(template)

	self:apply(entity, template)

	x = x or entity.x
	y = y or entity.y
	w = w or entity.w
	h = h or entity.h
	self.entitySys:setBounds(entity, x, y, w, h)

	return entity

end
function Template:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)

	self.simulation.constants.templates = {}
end
function Template:onRunTests()
	log.assert(self.simulation.constants.templates ~= nil)

	local template = self:add("yee", {
		["properties"] = {
			["x"] = 2,
		},
		["tags"] = {
			["yee"] = true
		},
	})
	log.assert(self:get("yee") == template)

	local entity = self:instantiate(template)
	log.assert(self.entitySys:find("yee") == entity)
	log.assert(self.entitySys:find("yee").x == 2)
	log.assert(self.entitySys:find("yee").tags["yee"] ~= nil)
end

return Template
