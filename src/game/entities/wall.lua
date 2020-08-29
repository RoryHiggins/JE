local SpriteSys = require("src/engine/sprite")
local TemplateSys = require("src/engine/template")

local WallSys = {}
SpriteSys.addSprite("wallMetal", 0, 40, 8, 8)
SpriteSys.addSprite("wallMetalVerticalLeft", 8, 40, 8, 8)
SpriteSys.addSprite("wallMetalVerticalMid", 16, 40, 8, 8)
SpriteSys.addSprite("wallMetalVerticalRight", 24, 40, 8, 8)
SpriteSys.addSprite("wallMetalHorizontalLeft", 0, 48, 8, 8)
SpriteSys.addSprite("wallMetalHorizontalMid", 0, 56, 8, 8)
SpriteSys.addSprite("wallMetalHorizontalRight", 0, 64, 8, 8)
SpriteSys.addSprite("wallMetalBackground", 32, 40, 8, 8)
WallSys.template = TemplateSys.add("wall", {
	["w"] = 8,
	["h"] = 8,
	["spriteId"] = "wallMetalBackground",
	["tags"] = {
		["sprite"] = true,
		["material"] = true,
		["solid"] = true,
	}
})

return WallSys
