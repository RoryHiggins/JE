local util = require("engine/util/util")
local log = require("engine/util/log")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")
local Background = require("engine/systems/background")
local Template = require("engine/systems/template")

local Placeholder = {}
Placeholder.SYSTEM_NAME = "placeholder"
function Placeholder:setTemplate(placeholder, template)
	log.assert(template.editor ~= nil)
	log.assert(template.properties.spriteId ~= nil)
	log.assert(template.properties.w ~= nil)
	log.assert(template.properties.h ~= nil)

	local sprite = self.spriteSys:get(template.properties.spriteId)
	log.assert(sprite ~= nil)

	for _, defaults in ipairs({sprite, template.properties}) do
		for _, key in ipairs(self.placeholderCopyFields) do
			if defaults[key] ~= nil then
				placeholder[key] = defaults[key]
			end
		end
	end

	placeholder.placeholderTemplateId = template.templateId
end
function Placeholder:create(template, x, y)
	local placeholder = self.templateSys:instantiate(self.placeholderTemplate, x, y)
	self:setTemplate(placeholder, template)

	return placeholder
end
function Placeholder:createIfFree(template, x, y, w, h)
	for _, placeholder in ipairs(self.entitySys:findAllBounded(x, y, w, h, "placeholder")) do
		if placeholder.placeholderTemplateId == template.templateId then
			return false
		end
	end

	self:create(template, x, y)
	return true
end
function Placeholder:clearArea(x, y, w, h)
	local changed = false
	for _, placeholder in ipairs(self.entitySys:findAllBounded(x, y, w, h, "placeholder")) do
		self.entitySys:destroy(placeholder)
		changed = true
	end
	return changed
end
function Placeholder:clearAll()
	local changed = false
	for _, placeholder in ipairs(self.entitySys:findAll("placeholder")) do
		self.entitySys:destroy(placeholder)
		changed = true
	end
	return changed
end
function Placeholder:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)

	self.placeholderTemplate = self.templateSys:add("placeholder", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["z"] = 1,
			["placeholderTemplateId"] = nil,
			["spriteId"] = self.spriteSys:getInvalid().spriteId,
		},
		["tags"] = {
			["sprite"] = true,
			["placeholder"] = true,
		},
	})
	self.placeholderCopyFields = {"w", "h", "r", "g", "b", "a", "spriteId"}
	self.placeholderVisible = true
end


local PlaceholderInstance = {}
PlaceholderInstance.SYSTEM_NAME = "placeholderInstance"
function PlaceholderInstance:clearAll()
	for _, editorPlaced in ipairs(self.entitySys:findAll(self.placeholderInstanceTag)) do
		self.entitySys:destroy(editorPlaced)
	end
end
function PlaceholderInstance:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)

	self.placeholderInstanceTag = "placeholderInstance"
end


local Editor = {}
Editor.EDITOR_FILENAME = "map_editor.world"
Editor.SYSTEM_NAME = "editor"

function Editor:findNextEditorTemplate(templateId, step)
	step = step or 1
	templateId = templateId or 1

	local templateCount = self.templateSys:getCount()
	log.assert(step ~= templateCount)

	log.trace("start: %s", templateId)
	local nextTemplateId = util.moduloAddSkipZero(templateId, step, templateCount + 1)
	while nextTemplateId ~= templateId do
		log.assert(templateId ~= 0)

		local template = self.templateSys:get(nextTemplateId)
		log.assert(template ~= nil)

		log.trace("candidate: %s", util.getComparable(template))

		if template.editor and template.editor.selectible then
			if self.spriteSys:get(template.properties.spriteId) == nil then
				log.error("selectible set for template with nonexistent spriteId: %s", util.getComparable(template))
			else
				return template
			end
		end

		nextTemplateId = util.moduloAddSkipZero(nextTemplateId, step, templateCount + 1)
	end

	return self.templateSys:get(templateId)
end
function Editor:setEditorTemplate(editor, template)
	local currentTemplate = self.templateSys:get(editor.placeholderTemplateId)

	log.debug("changing selection from %s to %s", currentTemplate.templateName, template.templateName)

	editor.placeholderTemplateId = template.templateId

	local sprite = self.spriteSys:get(template.properties.spriteId)
	log.assert(sprite ~= nil)

	for _, defaults in ipairs({sprite, template.properties}) do
		for _, key in ipairs(self.placeholderSys.placeholderCopyFields) do
			if defaults[key] ~= nil then
				editor[key] = defaults[key]
			end
		end
	end
end
function Editor:editModeStep()
	local editor = self:getInstance()

	local selectionScrollDir = (
		util.boolGetValue(self.inputSys:getReleased("right"))
		- util.boolGetValue(self.inputSys:getReleased("left")))

	local template = self.templateSys:get(editor.placeholderTemplateId)
	if selectionScrollDir == 0 and template.templateName == self.editorNoSelectionTemplate.templateName then
		selectionScrollDir = 1
	end

	if selectionScrollDir ~= 0 then
		self:setEditorTemplate(editor, self:findNextEditorTemplate(editor.placeholderTemplateId, selectionScrollDir))
		template = self.templateSys:get(editor.placeholderTemplateId)
	end

	if self.inputSys:get("mouseLeft") then
		if template.templateName ~= "editorNoSelection" then
			if self.placeholderSys:createIfFree(template, editor.x, editor.y, editor.w, editor.h) then
				self.saved = false
			end
		end
	end
	if self.inputSys:get("mouseRight") then
		if self.placeholderSys:clearArea(editor.x, editor.y, editor.w, editor.h) then
			self.saved = false
		end
	end
end
function Editor:editModeDraw(camera)
	local editor = self:getInstance()

	local template = self.templateSys:get(editor.placeholderTemplateId)
	self.textSys:drawDebugString("selection="..tostring(template.templateName))

	local editorOutline = util.deepcopy(editor)
	editorOutline.r = 0
	editorOutline.g = 0
	editorOutline.b = 0
	editorOutline.a = 0.5
	editorOutline.x = editorOutline.x - 1
	editorOutline.y = editorOutline.y - 1
	editorOutline.w = editorOutline.w + 2
	editorOutline.h = editorOutline.h + 2
	editorOutline.z = editorOutline.z - 1
	self.shapeSys:drawRect(editorOutline, camera, true)

	editor.x = math.floor((camera.mouseX) / self.gridSize) * self.gridSize
	editor.y = math.floor((camera.mouseY) / self.gridSize) * self.gridSize
end
function Editor:clearWorld()
	log.debug("")

	self.placeholderSys:clearAll()
	self.placeholderInstanceSys:clearAll()
	self.simulation:worldInit()

	self.saved = false
end
function Editor:clearSaveTable()
	self.saveTable = {
		["entities"] = {},
		["background"] = {
			["r"] = 1,
			["g"] = 1,
			["b"] = 1,
		}
	}
end
function Editor:saveToTable()
	log.debug("")

	local entities = {}
	local backgroundR, backgroundG, backgroundB = self.backgroundSys:getColor()
	local save = {
		["entities"] = entities,
		["background"] = {
			["r"] = backgroundR,
			["g"] = backgroundG,
			["b"] = backgroundB,
		}
	}
	for _, placeholder in ipairs(self.entitySys:findAll("placeholder")) do
		local template = self.templateSys:get(placeholder.placeholderTemplateId)
		if template ~= nil then
			local entity = {
				["x"] = placeholder.x,
				["y"] = placeholder.y,
				["templateName"] = template.templateName
			}
			entities[#entities + 1] = entity
		else
			log.error("unrecognized templateId=%s", placeholder.placeholderTemplateId)
		end
	end

	return save
end
function Editor:loadFromTable(save)
	log.debug("")

	self:clearWorld()
	self:clearSaveTable()
	for _, entity in ipairs(save.entities) do
		local template = self.templateSys:getByName(entity.templateName)
		if template ~= nil then
			if self.mode == "playing" then
				self.templateSys:instantiate(template, entity.x, entity.y)
			else
				self.placeholderSys:create(template, entity.x, entity.y)
			end
		else
			log.error("unrecognized templateId=%s", entity.templateName)
		end
	end

	local background = save.background
	self.backgroundSys:setColor(background.r, background.g, background.b)
end
function Editor:saveToFile(filename)
	log.info("filename=%s", filename)

	self.saveTable = self:saveToTable()

	if not util.writeDataUncompressed(filename, util.json.encode(self.saveTable)) then
		return false
	end

	self.saveFilename = filename
	self.saved = true
	return true
end
function Editor:loadFromFile(filename)
	log.info("filename=%s", filename)

	if not util.getFileExists(filename) then
		return false
	end

	self.saveFilename = filename

	local saveStr = util.readDataUncompressed(filename)
	if not saveStr then
		return false
	end

	local save = util.json.decode(saveStr)
	self:loadFromTable(save)

	self.saveTable = save
	self.saved = true
	return true
end
function Editor:getInstance()
	local editor = self.entitySys:find("editor")
	if not editor then
		editor = self.templateSys:instantiate(self.editorTemplate)
	end

	return editor
end
function Editor:setMode(newMode)
	local oldMode = self.mode

	if oldMode == newMode then
		return
	end

	log.debug("oldMode=%s, newMode=%s", oldMode, newMode)

	if oldMode == "editing" then
		self.entitySys:untag(self:getInstance(), "sprite")
	end

	if oldMode ~= "playing" then
		self.saveTable = self:saveToTable()
	end

	self.mode = newMode

	if (oldMode == "playing") or (newMode == "playing") then
		self:loadFromTable(self.saveTable)
	end

	if newMode == "editing" then
		self.entitySys:tag(self:getInstance(), "sprite")
	end
end
function Editor:startEditor()
	self.simulation:getSystem("player"):createWorld("editor")
	self:clearSaveTable()
	self:setMode("editing")
	self:loadFromFile(self.saveFilename)
end
function Editor:onInit(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.entitySys = self.simulation:addSystem(Entity)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)
	self.backgroundSys = self.simulation:addSystem(Background)
	self.templateSys = self.simulation:addSystem(Template)
	self.placeholderSys = self.simulation:addSystem(Placeholder)
	self.placeholderInstanceSys = self.simulation:addSystem(PlaceholderInstance)

	self.gridSize = 8

	self.editorNoSelectionTemplate = self.templateSys:add("editorNoSelection", {
		["properties"] = {
			["spriteId"] = self.spriteSys:getInvalid().spriteId
		},
		["editor"] = {
			["selectible"] = false,
			["category"] = "common"
		},
	})
	self.editorTemplate = self.templateSys:add("editor", {
		["properties"] = {
			["w"] = self.gridSize,
			["h"] = self.gridSize,
			["placeholderTemplateId"] = self.editorNoSelectionTemplate.templateId,
			["spriteId"] = self.spriteSys:getInvalid().spriteId,
		},
		["tags"] = {
			["editor"] = true,
			["sprite"] = true,
		},
	})
	self.modeIdToMode = {"editing", "saving", "loading", "playing"}
	self.modeToModeId = {}
	for i, mode in ipairs(self.modeIdToMode) do
		self.modeToModeId[mode] = i
	end

	self.mode = "playing"
	self:clearSaveTable()
	self.saved = false
	self.saveFilename = self.EDITOR_FILENAME
end
function Editor:onStep()
	if not self.simulation.constants.developerDebugging then
		return
	end

	local mouseReleased = (
		self.inputSys:getReleased("mouseLeft")
		or self.inputSys:getReleased("mouseRight")
	)
	local actionReleased = (
		self.inputSys:getReleased("a")
		or self.inputSys:getReleased("b")
	)
	local modeScrollDir = (
		util.boolGetValue(self.inputSys:getReleased("down"))
		- util.boolGetValue(self.inputSys:getReleased("up"))
	)

	if self.mode == "editing" then
		self:editModeStep()
	end

	if self.mode == "saving" then
		if actionReleased or mouseReleased then
			self:saveToFile(self.saveFilename)
		end
	end
	if self.mode == "loading" then
		if actionReleased or mouseReleased then
			self:loadFromFile(self.saveFilename)
		end
	end
	if self.mode == "playing" then
		if mouseReleased then
			self:setMode("editing")
		end
	end
	if self.mode ~= "playing" then
		local newModeId = util.moduloAddSkipZero(
			self.modeToModeId[self.mode],
			modeScrollDir,
			#self.modeIdToMode + 1
		)
		self:setMode(self.modeIdToMode[newModeId])
	end
end
function Editor:onCameraDraw(camera)
	if not self.simulation.constants.developerDebugging then
		return
	end

	self.textSys:drawDebugString("mode="..tostring(self.mode))

	if self.mode == "saving" then
		self.textSys:drawDebugString("saved="..tostring(self.saved))
	end

	if self.mode == "editing" then
		self:editModeDraw(camera)
	end
end

return Editor
