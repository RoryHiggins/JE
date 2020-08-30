local client = require("src/engine/client")
local System = require("src/engine/system")
local Entity = require("src/engine/entity")
local Screen = require("src/engine/screen")

local Text = System.new("text")
function Text:addFont(fontId, u, v, charW, charH, charFirst, charLast, charColumns)
	local fonts = self.simulation.static.fonts
	local font = fonts[fontId]
	if font == nil then
		font = {
			["fontId"] = fontId,
			["u"] = u,
			["v"] = v,
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
	return self.simulation.static.fonts[fontId]
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
function Text:onSimulationCreate()
	self:addDependencies(Entity, Screen)

	self.simulation.static.fonts = {}
end
function Text:onScreenDraw(screen)
	local fonts = self.simulation.static.fonts

	for _, entity in ipairs(self.entitySys:findAll("text")) do
		client.drawText(entity, fonts[entity.fontId], screen)
	end
end
function Text:onSimulationTests()
	local entity = self.entitySys:create()
	local testFont = self:addFont("test", 0, 192, 8, 8, " ", "_", 8)

	assert(self:getFont("test") == testFont)

	self:attach(entity, testFont, "hello")
	assert(entity.fontId == "test")
	assert(entity.tags.text)
	assert(entity.text == "hello")

	self:detach(entity, testFont)
	assert(entity.spriteId == nil)
	assert(entity.tags.sprite == nil)
	assert(entity.text == nil)
end

return Text
