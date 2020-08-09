-- local ClientSys = require("src/engine/client")
-- local SimulationSys = require("src/engine/simulation")
-- local EntitySys = require("src/engine/entity")
-- local ScreenSys = require("src/engine/screen")

-- SimulationSys.static.spriteFonts = {}

local TextSys = {}
-- function TextSys.addFont(fontId, u, v, charW, charH, charFirst, charLast, charColumns)
-- 	local spriteFonts = SimulationSys.static.spriteFonts
-- 	local spriteFont = spriteFonts[fontId]
-- 	if spriteFont == nil then
-- 		spriteFont = {
-- 			["fontId"] = fontId,
-- 			["u"] = u,
-- 			["v"] = v,
-- 			["charW"] = charW,
-- 			["charH"] = charH,
-- 			["charFirst"] = charFirst,
-- 			["charLast"] = charLast,
-- 			["charColumns"] = charColumns,
-- 		}
-- 		spriteFonts[fontId] = spriteFont
-- 	end

-- 	return spriteFont
-- end
-- function TextSys.attach(entity, spriteFont, text)
-- 	local spriteFontId = spriteFont.spriteFontId

-- 	entity.spriteFontId = spriteFontId
-- 	entity.text = text
-- 	EntitySys.tag(entity, "text")
-- end
-- table.insert(ScreenSys.drawEvents, function(screen)
-- 	local spriteFonts = SimulationSys.static.spriteFonts
-- 	local world = SimulationSys.state.world
-- 	local entities = world.entities

-- 	local spriteTextEntityIds = world.tagEntities["sprite"] or {}
-- 	local spriteTextEntityIdsCount = #spriteTextEntityIds

-- 	for i = 1, spriteTextEntityIdsCount do
-- 		local entity = entities[spriteTextEntityIds[i]]
-- 		local font = spriteFonts[entity.spriteFontId]
-- 		ClientSys.drawSprite(entity, font, screen)
-- 	end
-- end)

return TextSys
