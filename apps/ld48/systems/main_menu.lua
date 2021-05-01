local log = require("engine/util/log")
local util = require("engine/util/util")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Shape = require("engine/systems/shape")
local Template = require("engine/systems/template")
local Background = require("engine/systems/background")
local Editor = require("apps/ld48/systems/editor")

local MainMenu = {}
MainMenu.SYSTEM_NAME = "mainMenu"
function MainMenu:createMenuButton(text, handlerName)
	local buttonId = #self:getButtons() + 1
	local button = self.templateSys:instantiate(self.buttonTemplate)
	button.text = text
	button.buttonId = buttonId
	button.handlerName = handlerName
	button.x = 16
	button.y = (16 * buttonId)
	button.w, button.h = self.textSys:getTextSize(text)
end
function MainMenu:createMenuButtons()
	self:createMenuButton("New game", "newGamePressed")

	if util.getFileExists(self.simulation.SAVE_FILE) then
		self:createMenuButton("Continue game", "continueGamePressed")
	end

	-- if self.simulation.constants.developerDebugging then
		self:createMenuButton("Map editor", "editMapPressed")
	-- end
end
function MainMenu:newGamePressed()
	self.simulation:getSystem("player"):loadFirstWorld()
end
function MainMenu:continueGamePressed()
	self.simulation:load(self.simulation.SAVE_FILE)
end
function MainMenu:editMapPressed()
	self.editorSys:startEditor()
end
function MainMenu:getButtons()
	return self.entitySys:findAll("mainMenuButton")
end
function MainMenu:getSelectedButton()
	for _, button in ipairs(self:getButtons()) do
		if button.buttonId == self.selectedButtonId then
			return button
		end
	end
	return nil
end
function MainMenu:startMainMenu()
	log.info("setting up main menu")
	self.simulation:getSystem("player"):createWorld("mainMenu")
	self:createMenuButtons()
	self.backgroundSys:setColor(0.15, 0.15, 0.15)
end
function MainMenu:onInit(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.shapeSys = self.simulation:addSystem(Shape)
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.backgroundSys = self.simulation:addSystem(Background)
	self.editorSys = self.simulation:addSystem(Editor)

	self.selectedButtonId = 1
	self.font = self.textSys:getDefaultFont()

	self.buttonTemplate = self.templateSys:add("mainMenuButton", {
		["properties"] = {
			["r"] = 1,
			["g"] = 1,
			["b"] = 1,
			["text"] = "",
		},
		["tags"] = {
			["text"] = true,
			["mainMenuButton"] = true,
		},
	})
end
function MainMenu:onStep()
	local buttons = self:getButtons()
	if #buttons == 0 then
		return
	end

	local buttonScrollDir = (
		util.boolGetValue(self.inputSys:getReleased("down"))
		- util.boolGetValue(self.inputSys:getReleased("up"))
	)
	self.selectedButtonId = util.moduloAddSkipZero(
		self.selectedButtonId,
		buttonScrollDir,
		#buttons + 1
	)

	local actionReleased = (
		self.inputSys:getReleased("a")
		or self.inputSys:getReleased("b")
	)
	if actionReleased then
		local selectedButton = self:getSelectedButton()
		if not selectedButton then
			log.error("selectedButton doesn't exist, self=%s", util.getComparable(self))
			return
		end

		log.info("selectedButton.handlerName=%s", selectedButton.handlerName)

		local handler = self[selectedButton.handlerName]
		handler(self)
	end
end
function MainMenu:onCameraDraw(camera)
	local selectedButton = self:getSelectedButton()
	if not selectedButton then
		return
	end

	local buttonOutline = util.deepcopy(selectedButton)
	buttonOutline.r = 1
	buttonOutline.g = 1
	buttonOutline.b = 1
	buttonOutline.a = 0.5
	buttonOutline.x = buttonOutline.x - 1
	buttonOutline.y = buttonOutline.y - 1
	buttonOutline.w = buttonOutline.w + 2
	buttonOutline.h = buttonOutline.h + 2
	buttonOutline.z = buttonOutline.z - 1
	self.shapeSys:drawRect(buttonOutline, camera, true)
end

return MainMenu
