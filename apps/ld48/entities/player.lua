local log = require("engine/util/log")
local util = require("engine/util/util")
local Input = require("engine/systems/input")
local Entity = require("engine/systems/entity")
local Sprite = require("engine/systems/sprite")
local Text = require("engine/systems/text")
local Template = require("engine/systems/template")
local Editor = require("engine/systems/editor")

local Material = require("apps/ld48/systems/material")
local Physics = require("apps/ld48/systems/physics")

local Player = {}
Player.SYSTEM_NAME = "player"
Player.UNKNOWN_WORLD_NAME = "<unknown world>"
Player.UNKNOWN_WORLD_ID = 0
function Player:tickEntity(player)
	local constants = self.simulation.constants

	local materialPhysics = self.physicsSys:getMaterialPhysics(player)

	local inputDirX = (
		util.boolGetValue(self.inputSys:get("right"))
		- util.boolGetValue(self.inputSys:get("left")))
	local inputDirY = (
		util.boolGetValue(self.inputSys:get("down"))
		- util.boolGetValue(self.inputSys:get("up")))

	-- scale movement by normalized direction perpindicular to gravity (so movement=left/right when falling down, etc)
	local moveDirX = inputDirX * math.abs(util.sign(constants.physicsGravityY))
	local moveDirY = inputDirY * math.abs(util.sign(constants.physicsGravityX))

	local moveForceX = (moveDirX * player.playerMoveForce * materialPhysics.moveForceStrength)
	local moveForceY = (moveDirY * player.playerMoveForce * materialPhysics.moveForceStrength)

	local changingDirX = moveDirX ~= util.sign(player.speedX)
	local changingDirY = moveDirY ~= util.sign(player.speedY)

	if changingDirX then
		moveForceX = moveForceX * player.playerChangeDirForceMultiplier
	end
	if changingDirY then
		moveForceY = moveForceY * player.playerChangeDirForceMultiplier
	end

	if changingDirX or (math.abs(player.speedX) < player.playerTargetMovementSpeed) then
		moveForceX = moveForceX * player.playerBelowTargetMovementSpeedForceMultiplier
	end
	if changingDirY or (math.abs(player.speedY) < player.playerTargetMovementSpeed) then
		moveForceY = moveForceY * player.playerBelowTargetMovementSpeedForceMultiplier
	end

	player.forceX = player.forceX + moveForceX
	player.forceY = player.forceY + moveForceY

	local onGround = self.entitySys:findRelative(
		player,
		util.sign(constants.physicsGravityX),
		util.sign(constants.physicsGravityY),
		"solid"
	)
	local tryingToJump = (
		(self.inputSys:get("a"))
		or ((util.sign(constants.physicsGravityX) ~= 0) and (util.sign(inputDirX) == -util.sign(constants.physicsGravityX)))
		or ((util.sign(constants.physicsGravityY) ~= 0) and (util.sign(inputDirY) == -util.sign(constants.physicsGravityY)))
	)

	local fallingX = (constants.physicsGravityX ~= 0) and (player.speedX * util.sign(constants.physicsGravityX) >= 0)
	local fallingY = (constants.physicsGravityY ~= 0) and (player.speedY * util.sign(constants.physicsGravityY) >= 0)
	local falling = fallingX or fallingY
	if not tryingToJump or onGround or falling then
		player.playerJumpFramesCur = 0
	end

	if tryingToJump then
		if onGround then
			if constants.physicsGravityX ~= 0 then
				self.physicsSys:stopX(player)
			end
			if constants.physicsGravityY ~= 0 then
				self.physicsSys:stopY(player)
			end
			local jumpForce = player.playerJumpForce * materialPhysics.jumpForceStrength
			player.forceX = player.forceX - (util.sign(constants.physicsGravityX) * jumpForce)
			player.forceY = player.forceY - (util.sign(constants.physicsGravityY) * jumpForce)
			player.playerJumpFramesCur = player.playerJumpFrames
		elseif player.playerJumpFramesCur > 0 then
			local jumpFrameForce = player.playerJumpFrameForce * materialPhysics.jumpForceStrength
			player.forceX = player.forceX - (util.sign(constants.physicsGravityX) * jumpFrameForce)
			player.forceY = player.forceY - (util.sign(constants.physicsGravityY) * jumpFrameForce)
			player.playerJumpFramesCur = player.playerJumpFramesCur - 1
		end
	end

	local collidingWithDeath = self.entitySys:findBounded(
		player.x,
		player.y,
		player.w,
		player.h,
		"death"
	)
	local fallingToDeath = (player.y > (self.simulation.input.screen.y2 + 8))
	local dying = collidingWithDeath or fallingToDeath
	if dying then
		self:die(player)
	end

	-- if inputDirY < 0 then
	-- 	self.spriteSys:attach(player, self.spriteSys:get("playerUp"))
	-- elseif inputDirX > 0 then
	-- 	self.spriteSys:attach(player, self.spriteSys:get("playerRight"))
	-- else
	-- 	self.spriteSys:attach(player, self.spriteSys:get("playerLeft"))
	-- end
end
function Player:die()
	if self:getCurrentWorldName() == "editor" then
		self.editorSys:setMode(self.editorSys.modeEditing)
		return
	end

	if self:getIsProgressionMap() then
		self:reloadWorld()
		return
	end

	self.simulation:getSystem("mainMenu"):start()
end
function Player:resetProgress()
	self.simulation.state.player = {}
	self.simulation.state.player.worldName = self.UNKNOWN_WORLD_NAME
	self.simulation.state.player.worldId = self.UNKNOWN_WORLD_ID
end
function Player:getCurrentWorldName()
	return self.simulation.state.player.worldName
end
function Player:getIsProgressionMap()
	local worldId =  self.simulation.state.player.worldId
	local worldName = self.simulation.state.player.worldName
	return (
		(worldId ~= self.UNKNOWN_WORLD_ID)
		and (worldName ~= self.UNKNOWN_WORLD_NAME)
	)
end
function Player:computeWorldFilename(worldName)
	return string.format(self.simulation.constants.worldFilenameFormat, worldName)
end
function Player:getCurrentWorldFilename()
	return string.format(self.simulation.constants.worldFilenameFormat, self:getCurrentWorldName())
end
function Player:reloadWorld()
	log.assert(self:getIsProgressionMap())
	self:loadWorld(self.simulation.state.player.worldId)
end
function Player:startWorld(worldName, worldId)
	worldName = worldName or self.UNKNOWN_WORLD_NAME
	worldId = worldId or self.UNKNOWN_WORLD_ID

	log.info("travelling from world %s to world %s", self:getCurrentWorldName(), worldName)

	self.simulation.state.player.worldName = worldName
	self.simulation.state.player.worldId = worldId

	self.simulation:broadcast("onPlayerStartWorld", false, worldName)
end
function Player:createWorld(worldName, worldId)
	self.simulation:worldInit()
	self:startWorld(worldName, worldId)
end
function Player:loadWorld(worldId)
	log.assert(worldId >= 1)
	log.assert(worldId <= #self.simulation.constants.worlds)

	local worldName = self.simulation.constants.worlds[worldId]

	local worldFilename = self:computeWorldFilename(worldName)
	if not self.editorSys:loadFromFile(worldFilename) then
		log.error("failed to load file=%s", worldFilename)
		return false
	end

	self:startWorld(worldName, worldId)
end
function Player:hasNextWorld()
	return (self.simulation.state.player.worldId < #self.simulation.constants.worlds)
end
function Player:loadNextWorld()
	if not self:hasNextWorld() then
		return false
	end

	return self:loadWorld(self.simulation.state.player.worldId + 1)
end
function Player:onInit(simulation)
	self.simulation = simulation
	self.inputSys = self.simulation:addSystem(Input)
	self.entitySys = self.simulation:addSystem(Entity)
	self.spriteSys = self.simulation:addSystem(Sprite)
	self.textSys = self.simulation:addSystem(Text)
	self.templateSys = self.simulation:addSystem(Template)
	self.editorSys = self.simulation:addSystem(Editor)

	self.materialSys = self.simulation:addSystem(Material)
	self.physicsSys = self.simulation:addSystem(Physics)

	self.spriteSys:addSprite("player", 16, 0, 8, 8)
	self.template = self.templateSys:add("player", {
		["properties"] = {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "player",
			["playerJumpFramesCur"] = 0,
			["playerJumpForce"] = 4,
			["playerJumpFrameForce"] = 0.5,
			["playerJumpFrames"] = 15,
			["playerMoveForce"] = 0.25,
			["playerChangeDirForceMultiplier"] = 0.8,
			["playerTargetMovementSpeed"] = 2,
			["playerBelowTargetMovementSpeedForceMultiplier"] = 1.5,
			["physicsCanPush"] = true,
			["physicsCanCarry"] = false,
		},
		["tags"] = {
			["sprite"] = true,
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["player"] = true,
		},
		["editor"] = {
			["category"] = "common",
			["selectible"] = true,
		},
	})

	local constants = self.simulation.constants
	constants.worldFilenameFormat = "apps/ld48/data/%s.world"
	constants.worldFirst = 1
	constants.worldLast = 1
	constants.worlds = {
		"cave1"
	}

	self:resetProgress()
end
function Player:onLoadState()
	self:reloadWorld()
end
function Player:onStep()
	for _, player in ipairs(self.entitySys:findAll("player")) do
		self:tickEntity(player)
	end
end
function Player:onDraw()
	self.textSys:drawDebugString("world="..self:getCurrentWorldName())
end
function Player:onRunTests()
	self.templateSys:instantiate(self.template)
	for _ = 1, 10 do
		self:onStep()
	end
end
function Player:onStop()
	if self:getIsProgressionMap() then
		self.simulation:save(self.simulation.SAVE_FILE)
	end
end

return Player
