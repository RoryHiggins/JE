local UtilSys = require("src/engine/util")
local ClientSys = require("src/engine/client")
local SimulationSys = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")
local SpriteSys = require("src/engine/sprite")
local TemplateSys = require("src/engine/template")

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
	["playerJumpForce"] = 3,
	["playerJumpFrameForce"] = 0.5,
	["playerJumpFrames"] = 16,
	["playerTargetMovementSpeed"] = 3,
	["playerBelowTargetMovementSpeedMultiplier"] = 2,
	["playerChangeDirForceMultiplier"] = 1.5,
	["playerMoveForce"] = 0.6,
	["physicsCanPush"] = true,
	["physicsCanCarry"] = true,
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

	local inputDirX = UtilSys.boolToNumber(ClientSys.state.inputRight) - UtilSys.boolToNumber(ClientSys.state.inputLeft)
	local inputDirY = UtilSys.boolToNumber(ClientSys.state.inputDown) - UtilSys.boolToNumber(ClientSys.state.inputUp)

	-- scale movement by normalized direction perpindicular to gravity (so movement=left/right when falling down, etc)
	local gravityLength = math.sqrt((static.gravityX * static.gravityX) + (static.gravityY * static.gravityY))
	local moveDirX = inputDirX * (math.abs(static.gravityY) / gravityLength)
	local moveDirY = inputDirY * (math.abs(static.gravityX) / gravityLength)

	local moveForceX = (moveDirX * entity.playerMoveForce * materialPhysics.moveForceStrength)
	local moveForceY = (moveDirY * entity.playerMoveForce * materialPhysics.moveForceStrength)

	if moveDirX ~= UtilSys.sign(entity.speedX) then
		moveForceX = moveForceX * entity.playerChangeDirForceMultiplier
	end
	if moveDirY ~= UtilSys.sign(entity.speedY) then
		moveForceY = moveForceY * entity.playerChangeDirForceMultiplier
	end

	entity.forceX = entity.forceX + moveForceX
	entity.forceY = entity.forceY + moveForceY

	local onGround = EntitySys.findRelative(
		entity,
		UtilSys.sign(static.gravityX),
		UtilSys.sign(static.gravityY),
		"solid"
	)
	local tryingToJump = (
		((UtilSys.sign(static.gravityX) ~= 0) and (UtilSys.sign(inputDirX) == -UtilSys.sign(static.gravityX)))
		or ((UtilSys.sign(static.gravityY) ~= 0) and (UtilSys.sign(inputDirY) == -UtilSys.sign(static.gravityY)))
	)

	local fallingX = (static.gravityX ~= 0) and (entity.speedX * UtilSys.sign(static.gravityX) >= 0)
	local fallingY = (static.gravityY ~= 0) and (entity.speedY * UtilSys.sign(static.gravityY) >= 0)
	local falling = fallingX or fallingY
	if not tryingToJump or onGround or falling then
		entity.playerJumpFramesCur = 0
	end

	if tryingToJump then
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

	if inputDirY < 0 then
		SpriteSys.attach(entity, SpriteSys.getSprite("playerUp"))
	elseif inputDirX > 0 then
		SpriteSys.attach(entity, SpriteSys.getSprite("playerRight"))
	else
		SpriteSys.attach(entity, SpriteSys.getSprite("playerLeft"))
	end
end
table.insert(SimulationSys.stepEvents, function()
	for _, player in pairs(EntitySys.findAll("player")) do
		PlayerSys.tickEntity(player)
	end
end)

return PlayerSys
