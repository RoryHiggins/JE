local EngineSys = require("src/engine/engine")
local UtilSys = EngineSys.components.UtilSys
local SimulationSys = EngineSys.components.SimulationSys
local EntitySys = EngineSys.components.EntitySys
local SpriteSys = EngineSys.components.SpriteSys
local TemplateSys = EngineSys.components.TemplateSys

local PhysicsSys = require("src/game/physics")

local static = SimulationSys.static

local PlayerSys = {}
SpriteSys.addSprite("playerRight", 8 + 1, 0 + 2, 6, 6)
SpriteSys.addSprite("playerLeft", 16 + 1, 0 + 2, 6, 6)
SpriteSys.addSprite("playerUp", 24 + 1, 0 + 2, 6, 6)
PlayerSys.template = TemplateSys.add("player", {
	["w"] = 6,
	["h"] = 6,
	["spriteId"] = "playerRight",
	["playerJumpFramesCur"] = 0,
	["playerJumpForce"] = 5,
	["playerJumpFrameForce"] = 0.7,
	["playerJumpFrames"] = 15,
	["playerMoveForce"] = 0.6,
	["tags"] = {
		["sprite"] = true,
		["screenTarget"] = true,
		["material"] = true,
		["solid"] = true,
		["physics"] = true,
		["player"] = true,
	}
})
function PlayerSys.tickEntity(entity)
	local materialPhysics = PhysicsSys.getMaterialPhysics(entity)

	local moveForce = entity.playerMoveForce * materialPhysics.moveForceStrength
	if SimulationSys.inputs.left then
		entity.forceX = entity.forceX - moveForce
	end

	if SimulationSys.inputs.right then
		entity.forceX = entity.forceX + moveForce
	end

	local onGround = EntitySys.findRelative(
		entity,
		UtilSys.sign(static.gravityX),
		UtilSys.sign(static.gravityY),
		"solid"
	)
	local fallingX = (static.gravityX ~= 0) and (entity.speedX * UtilSys.sign(static.gravityX) >= 0)
	local fallingY = (static.gravityY ~= 0) and (entity.speedY * UtilSys.sign(static.gravityY) >= 0)
	local falling = fallingX or fallingY
	if not SimulationSys.inputs.up or onGround or falling then
		entity.playerJumpFramesCur = 0
	end

	if SimulationSys.inputs.up then
		if onGround then
			if static.gravityX ~= 0 then
				PhysicsSys.stopX(entity)
			end
			if static.gravityY ~= 0 then
				PhysicsSys.stopY(entity)
			end
			local jumpForce = entity.playerJumpForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (UtilSys.sign(static.gravityX) * jumpForce)
			entity.forceY = entity.forceY - (UtilSys.sign(static.gravityY) * jumpForce)
			entity.playerJumpFramesCur = entity.playerJumpFrames
		elseif entity.playerJumpFramesCur > 0 then
			local jumpFrameForce = entity.playerJumpFrameForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (UtilSys.sign(static.gravityX) * jumpFrameForce)
			entity.forceY = entity.forceY - (UtilSys.sign(static.gravityY) * jumpFrameForce)
			entity.playerJumpFramesCur = entity.playerJumpFramesCur - 1
		end
	end

	if entity.speedY < 0 then
		SpriteSys.attach(entity, SpriteSys.getSprite("playerUp"))
	else
		if entity.speedX < 0 then
			SpriteSys.attach(entity, SpriteSys.getSprite("playerLeft"))
		elseif entity.speedX > 0 then
			SpriteSys.attach(entity, SpriteSys.getSprite("playerRight"))
		elseif entity.spriteId == "playerUp" then
			SpriteSys.attach(entity, SpriteSys.getSprite("playerRight"))
		end
	end
end
table.insert(SimulationSys.stepEvents, function()
	for _, player in pairs(EntitySys.findAll("player")) do
		PlayerSys.tickEntity(player)
	end
end)

return PlayerSys
