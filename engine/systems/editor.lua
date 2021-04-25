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

	local oldW, oldH = placeholder.w, placeholder.h
	for _, defaults in ipairs({sprite, template.properties}) do
		for _, key in ipairs(self.placeholderCopyFields) do
			if defaults[key] ~= nil then
				placeholder[key] = defaults[key]
			end
		end
	end
	local newW, newH = placeholder.w, placeholder.h
	placeholder.w, placeholder.h = oldW, oldH
	self.entitySys:setSize(placeholder, newW, newH)

	placeholder.placeholderTemplateId = template.templateId
end
function Placeholder:create(template, x, y)
	local placeholder = self.templateSys:instantiate(self.placeholderTemplate, x, y)
	self:setTemplate(placeholder, template)

	return placeholder
end
function Placeholder:createIfFree(template, x, y, w, h)
	for _, placeholder in ipairs(self.entitySys:findAllBounded(x, y, w, h, "placeholder")) do
		local isSameTemplate = placeholder.placeholderTemplateId == template.templateId
		local isOverlappable = (template.editor.overlappable ~= nil) and template.editor.overlappable
		local equalBounds = (
				(x == placeholder.x)
				and (y == placeholder.y)
				and (w == placeholder.w)
				and (h == placeholder.h)
			)
		if isSameTemplate and (not isOverlappable or equalBounds) then
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
function Placeholder:getTemplatesInArea(x, y, w, h)
	local templates = {}
	for _, placeholder in ipairs(self.entitySys:findAllBounded(x, y, w, h, "placeholder")) do
		templates[#templates + 1] = self.templateSys:get(placeholder.placeholderTemplateId).templateName
	end

	return util.setCreate(templates)
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
	self.placeholderCopyFields = {"w", "h", "r", "g", "b", "a", "offsetX", "offsetY", "spriteId"}
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
Editor.modeEditing = "editing"
Editor.modeSaving = "saving"
Editor.modeLoading = "loading"
Editor.modeLevelSelect = "levelSelect"
Editor.modePlaying = "playing"

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
function Editor:unsetEditorTemplate(editor)
	self.entitySys:setSize(editor,  8, 8)
	editor.r = 1
	editor.g = 1
	editor.b = 1
	editor.a = 1
	editor.offsetX = 0
	editor.offsetY = 0
	editor.spriteId = self.spriteSys:getInvalid().spriteId
	editor.placeholderTemplateId = self.editorNoSelectionTemplate.templateId
end
function Editor:setEditorTemplate(editor, template)
	local currentTemplate = self.templateSys:get(editor.placeholderTemplateId)

	log.debug("changing selection from %s to %s", currentTemplate.templateName, template.templateName)

	self:unsetEditorTemplate(editor)

	editor.placeholderTemplateId = template.templateId

	local sprite = self.spriteSys:get(template.properties.spriteId)
	log.assert(sprite ~= nil)

	local oldW, oldH = editor.w, editor.h
	for _, defaults in ipairs({sprite, template.properties}) do
		for _, key in ipairs(self.placeholderSys.placeholderCopyFields) do
			if defaults[key] ~= nil then
				editor[key] = defaults[key]
			end
		end
	end
	local newW, newH = editor.w, editor.h
	editor.w, editor.h = oldW, oldH
	self.entitySys:setSize(editor, newW, newH)
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
	if self.inputSys:get("a") then
		local templatesAt = self.placeholderSys:getTemplatesInArea(editor.x, editor.y, editor.w, editor.h)
		if #templatesAt > 0 then
			self:setEditorTemplate(editor, self.templateSys:getByName(templatesAt[1]))
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

	local selectionTemplate = self.templateSys:get(editor.placeholderTemplateId)
	self.textSys:drawDebugString("brush="..tostring(selectionTemplate.templateName))

	local templatesAt = self.placeholderSys:getTemplatesInArea(editor.x, editor.y, editor.w, editor.h)
	if #templatesAt > 0 then
		local templatesStr = ""
		local first = true
		for _, templateName in ipairs(templatesAt) do
			if not first then
				templatesStr = templatesStr..","
			end
			templatesStr = templatesStr..templateName
			first = false
		end
		self.textSys:drawDebugString("hover="..templatesStr)
	end

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

	local x = math.floor((camera.mouseX) / self.gridSize) * self.gridSize
	local y = math.floor((camera.mouseY) / self.gridSize) * self.gridSize
	self.entitySys:setPos(editor, x, y)
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
		},
		["messages"] = util.deepcopy(self.messages)
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
	for _, entity in ipairs(save.entities or {}) do
		local template = self.templateSys:getByName(entity.templateName)
		if template ~= nil then
			if self.mode == self.modePlaying then
				self.templateSys:instantiate(template, entity.x, entity.y)
			else
				self.placeholderSys:create(template, entity.x, entity.y)
			end
		else
			log.error("unrecognized templateId=%s", entity.templateName)
		end
	end

	local messages = save.messages or {}
	local messageY = self.simulation.input.screen.y2 - (8 * #messages)
	for _, message in ipairs(save.messages or {}) do
		local messageEntity = self.templateSys:instantiate(self.messageTemplate, 0, messageY)
		messageEntity.text = message.text
		messageY = messageY + 8
	end
	self.messages = util.deepcopy(save.messages)

	local background = save.background or self.backgroundSys:getDefault()
	self.backgroundSys:setColor(background.r, background.g, background.b)

	self.saveTable = save
	self.saved = true
end
function Editor:saveToFile(filename)
	log.debug("filename=%s", filename)

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

	if oldMode == self.modeEditing then
		self.entitySys:untag(self:getInstance(), "sprite")
	end

	if newMode == self.modePlaying then
		self:saveToTable()
	end

	self.mode = newMode

	if (oldMode == self.modePlaying) or (newMode == self.modePlaying) then
		self:loadFromTable(self.saveTable)
	end

	if newMode == self.modeEditing then
		self.entitySys:tag(self:getInstance(), "sprite")
	end
end
function Editor:startEditor(world)
	self.simulation.constants.developerDebugging = true

	local playerSys = self.simulation:getSystem("player")
	if world ~= nil then
		self.saveFilename = world
		playerSys:loadWorld(world)
	else
		playerSys:createWorld("editor")
	end

	self:clearSaveTable()
	self:setMode(self.modeEditing)
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

	self.messageTemplate = self.templateSys:add("message", {
		["properties"] = {
			["r"] = 1,
			["g"] = 1,
			["b"] = 1,
			["a"] = 0.5,
			["text"] = "",
		},
		["tags"] = {
			["text"] = true,
			["message"] = true,
		},
	})

	self:setupModes()

	self.mode = self.modePlaying
	self:clearSaveTable()
	self.saved = false
	self.saveFilename = self.EDITOR_FILENAME
end
function Editor:setupModes()
	self.modeIdToMode = {}
	self.modeIdToMode[#self.modeIdToMode + 1] = self.modeEditing
	self.modeIdToMode[#self.modeIdToMode + 1] = self.modeLevelSelect
	self.modeIdToMode[#self.modeIdToMode + 1] = self.modePlaying
	self.modeToModeId = {}
	for i, mode in ipairs(self.modeIdToMode) do
		self.modeToModeId[mode] = i
	end

end
function Editor:onStep()
	if not self.simulation.constants.developerDebugging then
		return
	end

	local mouseReleased = self.inputSys:getReleased("mouseLeft")
	local actionReleased = (
		self.inputSys:getReleased("a")
		or self.inputSys:getReleased("b")
	)
	local modeScrollDir = (
		util.boolGetValue(self.inputSys:getReleased("down"))
		- util.boolGetValue(self.inputSys:getReleased("up"))
	)

	if self.mode == self.modeEditing then
		self:editModeStep()
		if not self.saved then
			self:saveToFile(self.saveFilename)
		end
	end

	-- if self.mode == self.modeSaving then
	-- 	if actionReleased or mouseReleased then
	-- 		self:saveToFile(self.saveFilename)
	-- 	end
	-- end
	-- if self.mode == self.modeLoading then
	-- 	if actionReleased or mouseReleased then
	-- 		self:loadFromFile(self.saveFilename)
	-- 	end
	-- end
	if self.mode == self.modeLevelSelect then
		if actionReleased or mouseReleased or self.inputSys:getReleased("right") then
			self.simulation:getSystem("player"):loadNextWorld()
			return
		end
		if self.inputSys:getReleased("left") then
			self.simulation:getSystem("player"):loadPrevWorld()
			return
		end
	end
	if self.mode == self.modePlaying then
		if mouseReleased or self.inputSys:getReleased("mouseMiddle") then
			self:setMode(self.modeEditing)
			return
		end
	end
	if self.mode ~= self.modePlaying then
		if self.inputSys:getReleased("mouseMiddle") then
			self:setMode(self.modePlaying)
			return
		end

		local newModeId = util.moduloAddSkipZero(
			self.modeToModeId[self.mode],
			modeScrollDir,
			#self.modeIdToMode + 1
		)
		local newMode = self.modeIdToMode[newModeId]
		if newMode ~= self.mode then
			self:setMode(self.modeIdToMode[newModeId])
			return
		end
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
