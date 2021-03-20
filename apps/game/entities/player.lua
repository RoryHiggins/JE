local util = require("engine/util/util")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")
local Sprite = require("engine/systems/sprite")
local Template = require("engine/systems/template")
local Physics = require("apps/game/physics")

local Player = {}
Player.SYSTEM_NAME = "player"
function Player:tickEntity(entity)
	local constants = self.simulation.constants

	local materialPhysics = self.physicsSys:getMaterialPhysics(entity)

	local inputDirX = (
		util.boolToNumber(self.inputSys:get("right"))
		- util.boolToNumber(self.inputSys:get("left")))
	local inputDirY = (
		util.boolToNumber(self.inputSys:get("down"))
		- util.boolToNumber(self.inputSys:get("up")))

	-- scale movement by normalized direction perpindicular to gravity (so movement=left/right when falling down, etc)
	local moveDirX = inputDirX * math.abs(util.sign(constants.physicsGravityY))
	local moveDirY = inputDirY * math.abs(util.sign(constants.physicsGravityX))

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
		util.sign(constants.physicsGravityX),
		util.sign(constants.physicsGravityY),
		"solid"
	)
	local tryingToJump = (
		(self.inputSys:get("a"))
		or ((util.sign(constants.physicsGravityX) ~= 0) and (util.sign(inputDirX) == -util.sign(constants.physicsGravityX)))
		or ((util.sign(constants.physicsGravityY) ~= 0) and (util.sign(inputDirY) == -util.sign(constants.physicsGravityY)))
	)

	local fallingX = (constants.physicsGravityX ~= 0) and (entity.speedX * util.sign(constants.physicsGravityX) >= 0)
	local fallingY = (constants.physicsGravityY ~= 0) and (entity.speedY * util.sign(constants.physicsGravityY) >= 0)
	local falling = fallingX or fallingY
	if not tryingToJump or onGround or falling then
		entity.playerJumpFramesCur = 0
	end

	if tryingToJump then
		if onGround then
			if constants.physicsGravityX ~= 0 then
				self.physicsSys:stopX(entity)
			end
			if constants.physicsGravityY ~= 0 then
				self.physicsSys:stopY(entity)
			end
			local jumpForce = entity.playerJumpForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (util.sign(constants.physicsGravityX) * jumpForce)
			entity.forceY = entity.forceY - (util.sign(constants.physicsGravityY) * jumpForce)
			entity.playerJumpFramesCur = entity.playerJumpFrames
		elseif entity.playerJumpFramesCur > 0 then
			local jumpFrameForce = entity.playerJumpFrameForce * materialPhysics.jumpForceStrength
			entity.forceX = entity.forceX - (util.sign(constants.physicsGravityX) * jumpFrameForce)
			entity.forceY = entity.forceY - (util.sign(constants.physicsGravityY) * jumpFrameForce)
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
function Player:onInit(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.entitySys = self.simulation:addSystem(Entity)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.templateSys = self.simulation:addSystem(Template)
	self.physicsSys = self.simulation:addSystem(Physics)

	self.spriteSys:addSprite("playerRight", 16 + 1, 0 + 2, 6, 6)
	self.spriteSys:addSprite("playerLeft", 24 + 1, 0 + 2, 6, 6)
	self.spriteSys:addSprite("playerUp", 32 + 1, 0 + 2, 6, 6)
	self.template = self.templateSys:add("player", {
		["properties"] = {
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
		},
		["tags"] = {
			["sprite"] = true,
			["cameraTarget"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["player"] = true,
		},
		["editor"] = {
			["category"] = "special",
		},
	})
end
function Player:onStep()
	for _, player in pairs(self.entitySys:findAll("player")) do
		self:tickEntity(player)
	end
end
function Player:onRunTests()
	self.templateSys:instantiate(self.template)
	for _ = 1, 10 do
		self:onStep()
	end
end

return Player
