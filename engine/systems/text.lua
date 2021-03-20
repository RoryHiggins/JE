local client = require("engine/client")
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
function Text.draw(_, renderable, font, camera)
	client.drawText(renderable, font, camera)
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
end
function Text:onCameraDraw(camera)
	local fonts = self.simulation.constants.fonts

	for _, entity in ipairs(self.entitySys:findAll("text")) do
		client.drawText(entity, fonts[entity.fontId], camera)
	end
end
function Text:onRunTests()
	local entity = self.entitySys:create()
	local testFont = self:addFont("test", 0, 160, 8, 8, " ", "~", 8)

	assert(self:getFont("test") == testFont)

	self:attach(entity, testFont, "hello")
	assert(entity.fontId == "test")
	assert(entity.tags.text)
	assert(entity.text == "hello")

	self.simulation:draw()

	self:detach(entity, testFont)
	assert(entity.spriteId == nil)
	assert(entity.tags.sprite == nil)
	assert(entity.text == nil)
end

return Text
