local log = require("engine/util/log")
local util = require("engine/util/util")
local client = require("engine/client/client")
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
Player.UNKNOWN_WORLD_NAME = "<world unset>"
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
	local nearGround = self.entitySys:findRelative(
		player,
		util.sign(constants.physicsGravityX),
		util.sign(constants.physicsGravityY) * player.playerDistanceNearGround,
		"solid"
	)
	local tryingToJump = (
		((util.sign(constants.physicsGravityX) ~= 0) and (util.sign(inputDirX) == -util.sign(constants.physicsGravityX)))
		or ((util.sign(constants.physicsGravityY) ~= 0) and (util.sign(inputDirY) == -util.sign(constants.physicsGravityY)))
	)
	local tryingToFall = (
		((util.sign(constants.physicsGravityX) ~= 0) and (util.sign(inputDirX) == util.sign(constants.physicsGravityX)))
		or ((util.sign(constants.physicsGravityY) ~= 0) and (util.sign(inputDirY) == util.sign(constants.physicsGravityY)))
	)

	local fallingX = (
		(constants.physicsGravityX ~= 0)
		and (player.speedX * util.sign(constants.physicsGravityX) >= 0)
	)
	local fallingY = (
		(constants.physicsGravityY ~= 0)
		and (player.speedY * util.sign(constants.physicsGravityY) >= 0)
	)
	local falling = (fallingX or fallingY) and not onGround

	local risingX = (
		(constants.physicsGravityX ~= 0)
		and (player.speedX * util.sign(constants.physicsGravityX) <= (-player.playerMovementDetectionThreshold))
	)
	local risingY = (
		(constants.physicsGravityY ~= 0)
		and (player.speedY * util.sign(constants.physicsGravityY) <= (-player.playerMovementDetectionThreshold))
	)
	local rising = risingX or risingY
	local shouldJump = tryingToJump and onGround and not rising
	local shouldHover = tryingToJump or (falling and not tryingToFall and not nearGround)

	if onGround and (player.playerHoverFramesCur < player.playerHoverFrames) then
		log.debug("player restore hover")
		player.playerHoverFramesCur = player.playerHoverFrames
	end

	if shouldJump then
		log.debug("player jump")
		if constants.physicsGravityX ~= 0 then
			self.physicsSys:stopX(player)
		end
		if constants.physicsGravityY ~= 0 then
			self.physicsSys:stopY(player)
		end
		local jumpForce = player.playerJumpForce * materialPhysics.jumpForceStrength
		player.forceX = player.forceX - (util.sign(constants.physicsGravityX) * jumpForce)
		player.forceY = player.forceY - (util.sign(constants.physicsGravityY) * jumpForce)
		player.playerHoverFramesCur = player.playerHoverFrames
		shouldHover = false
	end

	if shouldHover and (player.playerHoverFramesCur > 0) then
		log.trace("player hover, playerHoverFramesCur=%s", player.playerHoverFramesCur)
		local hoverFrameForce = player.playerHoverFrameForce * materialPhysics.jumpForceStrength
		player.forceX = player.forceX - (util.sign(constants.physicsGravityX) * hoverFrameForce)
		player.forceY = player.forceY - (util.sign(constants.physicsGravityY) * hoverFrameForce)
		player.playerHoverFramesCur = player.playerHoverFramesCur - 1
	end

	local collidingWithDeath = self.entitySys:findBounded(
		player.x,
		player.y,
		player.w,
		player.h,
		"death"
	)
	if collidingWithDeath then
		self:die(player)
	end

	local fallingOffMap = (player.y > (self.simulation.input.screen.y2 + 8))
	if fallingOffMap then
		log.debug("player off map")
		self:loadNextWorld()
	end

	local animationDir = "Mid"
	if (inputDirX < 0) then
		animationDir = "Left"
	elseif (inputDirX > 0) then
		animationDir = "Right"
	end

	local animationIndex = 1 + math.floor(client.state.frame / 7) % 3

	local spriteName = "player"..animationDir..tostring(animationIndex)
	self.spriteSys:attach(player, self.spriteSys:get(spriteName))

	local droneFloatOffsetY = -(math.floor(client.state.frame / 60) % 2)
	if not onGround then
		droneFloatOffsetY = 0
	end
	player.offsetY = droneFloatOffsetY
end
function Player:die()
	log.debug("player death")

	if self:getCurrentWorld() == "editor" then
		self.editorSys:setMode(self.editorSys.modeEditing)
		return
	end

	if self:getCurrentWorldIsTracked() then
		self:reloadWorld()
		return
	end

	self.loadFirstWorld()
end
function Player:resetProgress()
	self.simulation.state.player = {}
	self.simulation.state.player.worldName = self.UNKNOWN_WORLD_NAME
	self.simulation.state.player.worldId = self.UNKNOWN_WORLD_ID
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

	for i, dir in ipairs({"Mid", "Left", "Right"}) do
		local dirU = 16 + ((i - 1) * 3 * 8)
		for j = 1, 3 do
			local indexU = dirU + ((j - 1) * 8)
			local spriteName = "player"..dir..tostring(j)
			self.spriteSys:addSprite(spriteName, indexU, 0, 7, 7)
		end
	end
	self.template = self.templateSys:add("player", {
		["properties"] = {
			["w"] = 7,
			["h"] = 7,
			["spriteId"] = "playerRight2",
			["playerHoverFramesCur"] = 0,
			["playerHoverFrames"] = 20,
			["playerJumpForce"] = 2.5,
			["playerHoverFrameForce"] = 0.25,
			["playerMoveForce"] = 0.25,
			["playerMovementDetectionThreshold"] = 0.5,
			["playerDistanceNearGround"] = 4,
			["playerChangeDirForceMultiplier"] = 0.8,
			["playerTargetMovementSpeed"] = 2,
			["playerBelowTargetMovementSpeedForceMultiplier"] = 1.5,
			["physicsCanPush"] = true,
			["physicsCanCarry"] = false,
			["physicsGravityMultiplier"] = 0.25,
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
	constants.worldIdToWorld = {
		"cave1",
		"cave2",
		"cave3",
		"cave4",
		-- "cave5",
		-- "cave6",
		-- "cave7",
		-- "cave8",
		"temple1",
		"end",
	}
	constants.worldToWorldId = {}
	for i, mode in ipairs(constants.worldIdToWorld) do
		constants.worldToWorldId[mode] = i
	end
	constants.firstWorldId = 1
	constants.lastWorldId = #constants.worldIdToWorld

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
	self.textSys:drawDebugString("world="..self:getCurrentWorld())
end
function Player:onRunTests()
	self.templateSys:instantiate(self.template)
	for _ = 1, 10 do
		self:onStep()
	end
end
function Player:onStop()
	if self:getCurrentWorldIsTracked() then
		self.simulation:save(self.simulation.SAVE_FILE)
	else
		log.info("world not tracked, skipping saving, world=%s", self:getCurrentWorld())
	end
end

-- TODO: world management needs to move into engine
function Player:getCurrentWorld()
	return self.simulation.state.player.worldName
end
function Player:getCurrentWorldFilename()
	return string.format(self.simulation.constants.worldFilenameFormat, self:getCurrentWorld())
end
function Player:getCurrentWorldIsTracked()
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
function Player:reloadWorld()
	if not self:getCurrentWorldIsTracked() then
		log.error("trying to reload not tracked by world loader")
		self:loadFirstWorld()
		return
	end

	self:loadWorld(self:getCurrentWorld())
end
function Player:startWorld(worldName, worldId)
	worldName = worldName or self.UNKNOWN_WORLD_NAME
	worldId = worldId or self.UNKNOWN_WORLD_ID

	log.info("travelling from world %s to world %s", self:getCurrentWorld(), worldName)

	self.simulation.state.player.worldName = worldName
	self.simulation.state.player.worldId = worldId

	self.simulation:broadcast("onPlayerStartWorld", false, worldName)

	return true
end
function Player:createWorld(worldName, worldId)
	self.simulation:worldInit()
	self:startWorld(worldName, worldId)
end
function Player:loadWorld(world)
	log.assert(world ~= nil)
	if (world == nil) then
		return false
	end

	local worldId = self.simulation.constants.worldToWorldId[world]
	if (worldId == nil) then
		log.error("world not in list of worlds, world=%s", world)
		return false
	end

	local worldFilename = self:computeWorldFilename(world)
	if not self.editorSys:loadFromFile(worldFilename) then
		log.error("failed to load file=%s", worldFilename)
		return false
	end

	return self:startWorld(world, worldId)
end
function Player:loadWorldId(worldId)
	local constants = self.simulation.constants
	log.assert(worldId >= 1)
	log.assert(worldId <= #constants.worldIdToWorld)

	return self:loadWorld(constants.worldIdToWorld[worldId])
end
function Player:hasNextWorld()
	return (self.simulation.state.player.worldId < #self.simulation.constants.worldIdToWorld)
end
function Player:loadNextWorld()
	local nextWorldId = self.simulation.state.player.worldId + 1

	if not self:hasNextWorld() then
		log.info("no levels remain, wrapping around")
		nextWorldId = self.simulation.constants.firstWorldId
	end

	return self:loadWorldId(nextWorldId)
end
function Player:loadPrevWorld()
	local prevWorldId = self.simulation.state.player.worldId - 1

	if prevWorldId < self.simulation.constants.firstWorldId then
		log.info("no levels remain, wrapping around")
		prevWorldId = self.simulation.constants.lastWorldId
	end
	return self:loadWorldId(prevWorldId)
end
function Player:loadFirstWorld()
	return self:loadWorldId(self.simulation.constants.firstWorldId)
end

return Player
