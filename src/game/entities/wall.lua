local EngineSys = require("src/engine/engine")
local SpriteSys = EngineSys.components.SpriteSys
local TemplateSys = EngineSys.components.TemplateSys

local WallSys = {}
SpriteSys.addSprite("wallBlack", 0, 40, 8, 8)
WallSys.template = TemplateSys.add("wall", {
	["w"] = 8,
	["h"] = 8,
	["spriteId"] = "wallBlack",
	["tags"] = {
		["sprite"] = true,
		["material"] = true,
		["solid"] = true,
	}
})

return WallSys
