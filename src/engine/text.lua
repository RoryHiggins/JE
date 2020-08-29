-- local ClientSys = require("src/engine/client")
-- local SimulationSys = require("src/engine/simulation")
-- local EntitySys = require("src/engine/entity")
-- local ScreenSys = require("src/engine/screen")

-- SimulationSys.static.fonts = {}

-- local TextSys = {}
-- function TextSys.addFont(fontId, u, v, charW, charH, charFirst, charLast, charColumns)
-- 	local fonts = SimulationSys.static.fonts
-- 	local font = fonts[fontId]
-- 	if font == nil then
-- 		font = {
-- 			["fontId"] = fontId,
-- 			["u"] = u,
-- 			["v"] = v,
-- 			["charW"] = charW,
-- 			["charH"] = charH,
-- 			["charFirst"] = charFirst,
-- 			["charLast"] = charLast,
-- 			["charColumns"] = charColumns,
-- 		}
-- 		fonts[fontId] = font
-- 	end

-- 	return font
-- end
-- function TextSys.attach(entity, font, text)
-- 	local fontId = font.fontId

-- 	entity.fontId = fontId
-- 	entity.text = text
-- 	EntitySys.tag(entity, "text")
-- end
-- table.insert(ScreenSys.drawEvents, function(screen)
-- 	local fonts = SimulationSys.static.fonts
-- 	local world = SimulationSys.state.world
-- 	local entities = world.entities

-- 	local textEntityIds = EntitySys.findAll("text")
-- 	local textEntityIdsCount = #textEntityIds

-- 	for i = 1, textEntityIdsCount do
-- 		local entity = entities[textEntityIds[i]]
-- 		local font = fonts[entity.fontId]
-- 		ClientSys.drawSprite(entity, font, screen)
-- 	end
-- end)

-- return TextSys
