local log = require("engine/util/log")
local util = require("engine/util/util")
local client = require("engine/client/client")
local Entity = require("engine/systems/entity")
local Camera = require("engine/systems/camera")

local Text = {}
Text.SYSTEM_NAME = "text"
function Text:addFont(fontId, u, v, charW, charH, charFirst, charLast, charColumns)
	local fonts = self.simulation.constants.fonts
	local font = fonts[fontId]
	if font == nil then
		font = {
			["fontId"] = fontId,
			["u1"] = u,
			["v1"] = v,

			-- default to black text
			["r"] = 0,
			["g"] = 0,
			["b"] = 0,
			["a"] = 1,

			["charW"] = charW,
			["charH"] = charH,
			["charFirst"] = charFirst,
			["charLast"] = charLast,
			["charColumns"] = charColumns,
		}
		fonts[fontId] = font
	end

	return font
end
function Text:getFont(fontId)
	return self.simulation.constants.fonts[fontId]
end
function Text:getDefaultFont()
	return self.defaultFont
end
function Text.draw(_, renderable, font, camera)
	client.drawText(renderable, font, camera)
end
function Text:drawDebugString(text)
	if not self.simulation.constants.developerDebugging then
		return
	end

	local renderable = {
		["text"] = tostring(text),
		["x"] = 0,
		["y"] = self.debugTextY,
		["z"] = -10,
		["r"] = 0,
		["g"] = 0,
		["b"] = 0,
		["a"] = 0.4,
	}
	self.debugTextY = self.debugTextY + 8

	client.drawText(renderable, self.defaultFont, self.simulation.input.screen)

	renderable.y = renderable.y + 1
	renderable.r = 1
	renderable.g = 1
	renderable.b = 1
	client.drawText(renderable, self.defaultFont, self.simulation.input.screen)
end
function Text:getTextSize(text, font)
	font = font or self:getDefaultFont()

	return (#text * font.charW), (font.charH)
end
function Text:attach(entity, font, text)
	entity.fontId = font.fontId
	entity.text = text
	self.entitySys:tag(entity, "text")
end
function Text:detach(entity)
	self.entitySys:untag(entity, "text")
	entity.text = nil
	entity.fontId = nil
end
function Text:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
	self.cameraSys = self.simulation:addSystem(Camera)

	self.simulation.constants.fonts = {}

	self.debugTextY = 0

	self.defaultFont = self:addFont("default", 0, 160, 8, 8, " ", "~", 8)
end
function Text:onStep()
	self.debugTextY = 0
end
function Text:onCameraDraw(camera)
	local fonts = self.simulation.constants.fonts

	for _, entity in ipairs(self.entitySys:findAll("text")) do

		local font = self:getDefaultFont()
		if entity.fontId ~= nil then
			local entityFont = fonts[entity.fontId]
			if font == nil then
				log.error("unknown font, entity=%s", util.getComparable(entity))
			else
				font = entityFont
			end
		end

		self:draw(entity, font, camera)
	end
end
function Text:onRunTests()
	local entity = self.entitySys:create()
	local testFont = self:addFont("test", 0, 160, 8, 8, " ", "~", 8)

	log.assert(self:getFont("test") == testFont)

	self:attach(entity, testFont, "hello")
	log.assert(entity.fontId == "test")
	log.assert(entity.tags.text)
	log.assert(entity.text == "hello")

	self.simulation:draw()

	self:detach(entity, testFont)
	log.assert(entity.spriteId == nil)
	log.assert(entity.tags.sprite == nil)
	log.assert(entity.text == nil)
end

return Text
