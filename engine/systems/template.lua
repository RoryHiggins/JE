local log = require("engine/util/log")
local util = require("engine/util/util")
local Entity = require("engine/systems/entity")

local Template = {}
Template.SYSTEM_NAME = "template"
function Template:add(templateName, template)
	local constants = self.simulation.constants
	local templatesByName = constants.templatesByName
	local templatesById = constants.templatesById

	local currentTemplate = templatesByName[templateName]
	if currentTemplate ~= nil then
		return currentTemplate
	end

	local templateId = self:getCount() + 1
	template.templateName = templateName
	template.templateId = templateId

	templatesByName[templateName] = template
	templatesById[templateId] = template

	return template
end
function Template:get(templateId)
	return self.simulation.constants.templatesById[templateId]
end
function Template:getCount()
	return #self.simulation.constants.templatesById
end
function Template:getByName(templateName)
	return self.simulation.constants.templatesByName[templateName]
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
	log.assert(template ~= nil)

	local entity = self.entitySys:create()
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

	local constants = self.simulation.constants
	constants.templatesByName = {}
	constants.templatesById = {}
end
function Template:onRunTests()
	log.assert(self.simulation.constants.templatesByName ~= nil)
	log.assert(self.simulation.constants.templatesById ~= nil)

	local template = self:add("yee", {
		["properties"] = {
			["x"] = 2,
		},
		["tags"] = {
			["yee"] = true
		},
	})
	log.assert(self:getByName("yee") == template)
	log.assert(self:get(template.templateId) == template)

	local entity = self:instantiate(template)
	log.assert(self.entitySys:find("yee") == entity)
	log.assert(self.entitySys:find("yee").x == 2)
	log.assert(self.entitySys:find("yee").tags["yee"] ~= nil)
end

return Template
