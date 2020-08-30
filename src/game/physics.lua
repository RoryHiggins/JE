local util = require("src/engine/util")
local Simulation = require("src/engine/simulation")
local Entity = require("src/engine/entity")
local Template = require("src/engine/template")
local Material = require("src/game/material")

local Physics = Simulation.createSystem("physics")
function Physics:getMaterialPhysics(entity)
	local static = self.simulation.static

	local gravitySignX = util.sign(static.physicsGravityX)
	local gravitySignY = util.sign(static.physicsGravityY)
	local materialEntity = self.entitySys:findBounded(
		entity.x + math.min(0, gravitySignX),
		entity.y + math.min(0, gravitySignY),
		entity.w + math.max(0, gravitySignX),
		entity.h + math.max(0, gravitySignY),
		"material",
		entity.id
	)

	local materialsPhysics = static.physicsMaterials
	local materialPhysics = materialsPhysics.air
	if materialEntity then
		local entityTags = materialEntity.tags

		local materials = static.materials
		local materialsCount = #materials
		for i = 1, materialsCount do
			local material = materials[i]
			local candidateMaterialPhysics = materialsPhysics[material]
			if entityTags[material] and candidateMaterialPhysics then
				materialPhysics = candidateMaterialPhysics
				break
			end
		end
	end
	return materialPhysics
end
function Physics:getCarryablesRecursive(entity, outCarryables, recursionDepth)
	outCarryables = outCarryables or {}

	local static = self.simulation.static

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return outCarryables
	end

	local candidates = self.entitySys:findAllRelative(
		entity, -util.sign(static.physicsGravityX), -util.sign(static.physicsGravityY), "physicsCarryable")
	local candidatesCount = #candidates

	for i = 1, candidatesCount do
		local candidate = candidates[i]
		local entityId = candidate.id
		if outCarryables[entityId] == nil then
			outCarryables[entityId] = candidate
			self:getCarryablesRecursive(candidate, outCarryables, recursionDepth + 1)
		end
	end

	return outCarryables
end
function Physics.stopX(_, entity)
	entity.forceX = 0
	entity.speedX = 0
	entity.overflowX = 0
end
function Physics.stopY(_, entity)
	entity.forceY = 0
	entity.speedY = 0
	entity.overflowY = 0
end
function Physics:tryPushX(entity, signX, recursionDepth)
	local static = self.simulation.static

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	if not entity.tags.physics then
		return false
	end

	if not entity.tags.physicsPushable then
		return false
	end

	if not self:tryMoveX(entity, signX, recursionDepth + 1, (recursionDepth ~= 1)) then
		return false
	end

	return true
end
function Physics:tryPushY(entity, signY, recursionDepth)
	local static = self.simulation.static

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	if not entity.tags.physics then
		return false
	end

	if not entity.tags.physicsPushable then
		return false
	end

	if not self:tryMoveY(entity, signY, recursionDepth + 1, (recursionDepth ~= 1)) then
		return false
	end

	return true
end
function Physics:tryMoveX(entity, moveX, recursionDepth, innerMove)
	local static = self.simulation.static

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	local signX = util.sign(moveX)
	local absMoveX = math.min(static.physicsMaxSpeed, math.abs(moveX))
	local moveSuccessful = true

	local curMoveX = 0
	for _ = 1, absMoveX do
		local nextMoveX = curMoveX + signX

		local obstacle = self.entitySys:findRelative(entity, nextMoveX, 0, "solid")
		while obstacle and entity.physicsCanPush and self:tryPushX(obstacle, signX, recursionDepth + 1) do
			recursionDepth = recursionDepth + 1
			entity.forceX = entity.forceX - (signX * static.physicsPushCounterforce)
			obstacle = self.entitySys:findRelative(entity, nextMoveX, 0, "solid")
			recursionDepth = recursionDepth + 1
		end
		if obstacle then
			self:stopX(entity)
			moveSuccessful = false
			break
		end

		curMoveX = nextMoveX
	end

	self.entitySys:setBounds(entity, entity.x + curMoveX, entity.y, entity.w, entity.h)

	if curMoveX ~= 0 and entity.physicsCanCarry and not innerMove and static.physicsGravityY ~= 0 then
		for _, carryable in pairs(self:getCarryablesRecursive(entity, {}, recursionDepth + 1)) do
			self:tryMoveX(carryable, curMoveX, recursionDepth + 1, true)
		end
	end

	return moveSuccessful
end
function Physics:tryMoveY(entity, moveY, recursionDepth, innerMove)
	local static = self.simulation.static

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	local signY = util.sign(moveY)
	local absMoveY = math.min(static.physicsMaxSpeed, math.abs(moveY))
	local moveSuccessful = true

	local curMoveY = 0
	for _ = 1, absMoveY do
		local nextMoveY = curMoveY + signY

		local obstacle = self.entitySys:findRelative(entity, 0, nextMoveY, "solid")
		while obstacle and entity.physicsCanPush and self:tryPushY(obstacle, signY, recursionDepth + 1) do
			recursionDepth = recursionDepth + 1
			entity.forceY = entity.forceY - (signY * static.physicsPushCounterforce)
			obstacle = self.entitySys:findRelative(entity, 0, nextMoveY, "solid")
		end
		if obstacle then
			self:stopY(entity)
			moveSuccessful = false
			break
		end

		curMoveY = nextMoveY
	end

	if curMoveY ~= 0 and entity.physicsCanCarry and not innerMove and static.physicsGravityX ~= 0 then
		for _, carryable in pairs(self:getCarryablesRecursive(entity, {}, recursionDepth + 1)) do
			self:tryMoveY(carryable, curMoveY, recursionDepth + 1, true)
		end
	end

	self.entitySys:setBounds(entity, entity.x, entity.y + curMoveY, entity.w, entity.h)

	return moveSuccessful
end
function Physics:tickForces(entity)
	local static = self.simulation.static

	-- apply force to speed
	entity.speedX = entity.speedX + entity.forceX
	entity.speedY = entity.speedY + entity.forceY
	entity.forceX = 0
	entity.forceY = 0

	-- get material physics to apply
	local materialPhysics = self:getMaterialPhysics(entity)

	-- apply gravity to force
	entity.forceX = entity.forceX + static.physicsGravityX
	entity.forceY = entity.forceY + static.physicsGravityY

	-- apply "friction" to speed
	local speedSignX = util.sign(entity.speedX)
	local speedSignY = util.sign(entity.speedY)
	entity.speedX = entity.speedX - (materialPhysics.friction * util.sign(entity.speedX))
	entity.speedY = entity.speedY - (materialPhysics.friction * util.sign(entity.speedY))
	if util.sign(entity.speedX) ~= speedSignX then
		self:stopX(entity)
	end
	if util.sign(entity.speedY) ~= speedSignY then
		self:stopY(entity)
	end
end
function Physics:tickMovement(entity)
	local static = self.simulation.static

	-- clamp speed to max speed
	entity.speedX = math.max(-static.physicsMaxSpeed, math.min(static.physicsMaxSpeed, entity.speedX))
	entity.speedY = math.max(-static.physicsMaxSpeed, math.min(static.physicsMaxSpeed, entity.speedY))

	-- compute amount to move (integer values).  the fractional movement component is accumulated for subsequent ticks
	local moveX, overflowX = math.modf(entity.speedX)
	local moveY, overflowY = math.modf(entity.speedY)
	local overflowCarryX, overflowRemainderX = math.modf(overflowX + entity.overflowX)
	local overflowCarryY, overflowRemainderY = math.modf(overflowY + entity.overflowY)
	entity.overflowX = overflowRemainderX
	entity.overflowY = overflowRemainderY
	moveX = moveX + overflowCarryX
	moveY = moveY + overflowCarryY

	local signX = util.sign(moveX)
	local signY = util.sign(moveY)
	local movingX = (signX ~= 0)
	local movingY = (signY ~= 0)
	local absMoveX = math.abs(moveX)
	local absMoveY = math.abs(moveY)
	for i = 1, math.max(absMoveX, absMoveY) do
		if not movingX and not movingY then
			break
		end

		if movingX then
			movingX = self:tryMoveX(entity, signX) and (i < absMoveX)
		end
		if movingY then
			movingY = self:tryMoveY(entity, signY) and (i < absMoveY)
		end
	end
end
function Physics:tick(entity)
	self:tickForces(entity)
	self:tickMovement(entity)
end
function Physics:onSimulationCreate()
	self.entitySys = self.simulation:addSystem(Entity)
	self.templateSys = self.simulation:addSystem(Template)
	self.materialSys = self.simulation:addSystem(Material)

	local static = self.simulation.static

	static.physicsGravityX = 0
	static.physicsGravityY = 0.5
	static.physicsMaxSpeed = 8
	static.physicsMaxRecursionDepth = 100
	static.physicsPushCounterforce = 0.1

	local defaultMaterialPhysics = {
		["friction"] = 0.3,
		["moveForceStrength"] = 1,
		["jumpForceStrength"] = 1,
	}
	static.physicsMaterials = {}
	for _, materialName in ipairs(self.simulation.static.materials) do
		static.physicsMaterials[materialName] = util.tableExtend({["id"] = materialName}, defaultMaterialPhysics)
	end
	local airPhysics = static.physicsMaterials.air
	airPhysics.friction = 0.1
	airPhysics.moveForceStrength = 0.5
end
function Physics:onSimulationStep()
	for _, entity in ipairs(self.entitySys:findAll("physics")) do
		self:tick(entity)
	end
end
function Physics.onEntityTag(_, entity, tag, tagId)
	if tagId ~= nil and tag == "physics" then
		entity.forceX = entity.forceX or 0
		entity.forceY = entity.forceY or 0
		entity.speedX = entity.speedX or 0
		entity.speedY = entity.speedY or 0

		-- overflow from last physics tick.  an artefact of locking x, y to integer values
		entity.overflowX = entity.overflowX or 0
		entity.overflowY = entity.overflowY or 0

		entity.physicsCanPush = entity.physicsCanPush or false
		entity.physicsCanCarry = entity.physicsCanCarry or false
	end
end
function Physics:onSimulationTests()
	local static = self.simulation.static

	local gridSize = 8

	local gravitySignX = util.sign(static.physicsGravityX)
	local gravitySignY = util.sign(static.physicsGravityY)

	local gridDownX = gridSize * gravitySignX
	local gridDownY = gridSize * gravitySignY

	local physicsTemplate = self.templateSys:add("physicsTest", {
		["w"] = gridSize,
		["h"] = gridSize,
		["physicsCanPush"] = true,
		["physicsCanCarry"] = true,
		["tags"] = {
			["material"] = true,
			["solid"] = true,
			["physics"] = true,
			["physicsPushable"] = true,
			["physicsCarryable"] = true,
			["physicsObject"] = true,
		}
	})
	local wallTemplate = self.templateSys:add("physicsWallTest", {
		["w"] = gridSize,
		["h"] = gridSize,
		["tags"] = {
			["material"] = true,
			["solid"] = true,
		}
	})

	-- sanity check; physics system only supports gravity on one axis
	assert((gravitySignX == 0) or (gravitySignY == 0))

	-- simulate being in the air
	local entity = self.templateSys:instantiate(physicsTemplate, 0, 0)
	assert(self:getMaterialPhysics(entity) == static.physicsMaterials["air"])

	-- simulate being on the ground
	self.templateSys:instantiate(wallTemplate, gridDownX, gridDownY)
	assert(self:getMaterialPhysics(entity) == static.physicsMaterials["solid"])

	-- simulate idle frames
	assert(entity.x == 0)
	assert(entity.y == 0)
	assert(entity.forceX == 0)
	assert(entity.forceY == 0)
	assert(entity.speedX == 0)
	assert(entity.speedY == 0)
	for _ = 1, 5 do
		self:onSimulationStep()
		assert(entity.x == 0)
		assert(entity.y == 0)
	end

	-- simulate falling off a ledge
	for _ = 1, 100 do
		entity.forceX = entity.forceX + (static.physicsGravityY)
		entity.forceY = entity.forceY + (static.physicsGravityX)
		self:onSimulationStep()
	end
	assert(util.sign(entity.x) ~= 0)
	assert(util.sign(entity.y) ~= 0)

	-- test reset logic
	self:stopX(entity)
	self:stopY(entity)
	self.entitySys:setPos(entity, 0, 0)
	assert(entity.x == 0)
	assert(entity.y == 0)
	assert(entity.forceX == 0)
	assert(entity.forceY == 0)
	assert(entity.speedX == 0)
	assert(entity.speedY == 0)

	-- simulate being pushed
	assert(self:tryPushX(entity, 1) or self:tryPushY(entity, 1))
	assert((entity.x ~= 0) or (entity.y ~= 0))

	-- simulate being carried
	local carryable = self.templateSys:instantiate(physicsTemplate, -gridDownX, -gridDownY)
	assert(self:tryMoveX(entity, 1) or self:tryMoveY(entity, 1))
	assert((entity.x ~= 0) or (entity.y ~= 0))
	assert((carryable.x ~= -gridDownX) or (carryable.y ~= -gridDownY))
end

return Physics

