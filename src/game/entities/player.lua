local util = require("src/engine/util")
local System = require("src/engine/system")
local Entity = require("src/engine/entity")
local Sprite = require("src/engine/sprite")
local Template = require("src/engine/template")

local Physics = require("src/game/physics")

local Player = System.new("player")
function Player:tickEntity(entity)
	local static = self.simulation.static

	local materialPhysics = self.physicsSys:getMaterialPhysics(entity)

	local inputDirX = (
		util.boolToNumber(self.simulation.inputs.right.down)
		- util.boolToNumber(self.simulation.inputs.left.down))
	local inputDirY = (
		util.boolToNumber(self.simulation.inputs.down.down)
		- util.boolToNumber(self.simulation.inputs.up.down))

	-- scale movement by normalized direction perpindicular to gravity (so movement=left/right when falling down, etc)
	local moveDirX = inputDirX * math.abs(util.sign(static.physicsGravityY))
	local moveDirY = inputDirY * math.abs(util.sign(static.physicsGravityX))

	local moveForceX = (moveDirX * entity.playerMoveForce * materialPhysics.moveForceStrength)
	local moveForceY = (moveDirY * entity.playerMoveForce * materialPhysics.moveForceStrength)

	local changingDirX = moveDirX ~= util.sign(entity.speedX)
	local changingDirY = moveDirY ~= util.sign(entity.speedY)

	if changingDirX then
		moveForceX = moveForceX * entity.playerChangeDirForceMultiplier
	end
	if changingDirY then
		moveForceY = moveForceY * entity.playerChangeDirForceMultiplier
	end

	if changingDirX or (math.abs(entity.speedX) < entity.playerTargetMovementSpeed) then
		moveForceX = moveForceX * entity.playerBelowTargetMovementSpeedForceMultiplier
	end
	if changingDirY or (math.abs(entity.speedY) < entity.playerTargetMovementSpeed) then
		moveForceY = moveForceY * entity.playerBelowTargetMovementSpeedForceMultiplier
	end

	entity.forceX = entity.forceX + moveForceX
	entity.forceY = entity.forceY + moveForceY

	local onGround = self.entitySys:findRelative(
		entity,
		util.sign(static.physicsGravityX),
		util.sign(static.physicsGravityY),
		"solid"
	)
	local tryingToJump = (
		((util.sign(static.physicsGravityX) ~= 0) and (util.sign(inputDirX) == -util.sign(static.physicsGravityX)))
		or ((util.sign(static.physicsGravityY) ~= 0) and (util.sign(inputDirY) == -util.sign(static.physicsGravityY)))
	)

	local fallingX = (static.physicsGravityX ~= 0) and (entity.speedX * util.sign(static.physicsGravityX) >= 0)
	local fallingY = (static.physicsGravityY ~= 0) and (entity.speedY * util.sign(static.physicsGravityY) >= 0)
	local falling = fallingX or fallingY
	if not tryingToJump or onGround or falling then
		entity.playerJumpFramesCur = 0
	end

	if tryingToJump then
		if onGround then
			if static.physicsGravityX ~= 0 then
				self.physicsSys:stopX(entity)
			end
			if static.physicsGravityY ~= 0 then
				self.physicsSys:stopY(entity)
			end
			local jumpForce = entity.playerJumpForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (util.sign(static.physicsGravityX) * jumpForce)
			entity.forceY = entity.forceY - (util.sign(static.physicsGravityY) * jumpForce)
			entity.playerJumpFramesCur = entity.playerJumpFrames
		elseif entity.playerJumpFramesCur > 0 then
			local jumpFrameForce = entity.playerJumpFrameForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (util.sign(static.physicsGravityX) * jumpFrameForce)
			entity.forceY = entity.forceY - (util.sign(static.physicsGravityY) * jumpFrameForce)
			entity.playerJumpFramesCur = entity.playerJumpFramesCur - 1
		end
	end

	if inputDirY < 0 then
		self.spriteSys:attach(entity, self.spriteSys:get("playerUp"))
	elseif inputDirX > 0 then
		self.spriteSys:attach(entity, self.spriteSys:get("playerRight"))
	else
		self.spriteSys:attach(entity, self.spriteSys:get("playerLeft"))
	end
end
function Player:onSimulationCreate()
	self:addDependencies(Entity, Sprite, Template, Physics)

	self.spriteSys:addSprite("playerRight", 8 + 1, 0 + 2, 6, 6)
	self.spriteSys:addSprite("playerLeft", 16 + 1, 0 + 2, 6, 6)
	self.spriteSys:addSprite("playerUp", 24 + 1, 0 + 2, 6, 6)
	self.template = self.templateSys:add("player", {
		["w"] = 6,
		["h"] = 6,
		["spriteId"] = "playerRight",
		["playerJumpFramesCur"] = 0,
		["playerJumpForce"] = 4,
		["playerJumpFrameForce"] = 0.5,
		["playerJumpFrames"] = 15,
		["playerMoveForce"] = 0.25,
		["playerChangeDirForceMultiplier"] = 0.8,
		["playerTargetMovementSpeed"] = 2,
		["playerBelowTargetMovementSpeedForceMultiplier"] = 1.5,
		["physicsCanPush"] = true,
		["physicsCanCarry"] = false,  -- it is tremendously annoying for a player; crates stick to your head
		["tags"] = {
			["sprite"] = true,
			["screenTarget"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["player"] = true,
		}
	})
end
function Player:onSimulationStep()
	for _, player in pairs(self.entitySys:findAll("player")) do
		self:tickEntity(player)
	end
end
function Player:onSimulationTests()
	self:onSimulationStep()

	self.templateSys:instantiate(self.template)
	for _ = 1, 10 do
		self:onSimulationStep()
	end
end

return Player
