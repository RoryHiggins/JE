local UtilSys = require("src/engine/util")
local SimulationSys = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")

local mathAbs = math.abs
local mathMin = math.min
local mathMax = math.max
local mathModf = math.modf
local UtilSysSign = UtilSys.sign
local EntitySysFindRelative = EntitySys.findRelative
local EntitySysFindAllRelative = EntitySys.findAllRelative
local EntitySysSetBounds = EntitySys.setBounds

local static = SimulationSys.static


local defaultMaterialPhysics = {
	["friction"] = 0.3,
	["moveForceStrength"] = 1,
	["jumpForceStrength"] = 1,
}
static.physicsPushCounterforce = 0.1
static.physicsGravityX = 0
static.physicsGravityY = 0.5
static.physicsMaxSpeed = 8
static.physicsMaxRecursionDepth = 25
static.physicsMaterials = {
	["air"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {
		["friction"] = 0.1,
		["moveForceStrength"] = 0.5,
	}),
	["solid"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["water"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["ice"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["death"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
}

local PhysicsSys = {}
function PhysicsSys.getMaterialPhysics(entity)
	local gravitySignX = UtilSysSign(static.physicsGravityX)
	local gravitySignY = UtilSysSign(static.physicsGravityY)
	local materialEntity = EntitySys.findBounded(
		entity.x + mathMin(0, gravitySignX),
		entity.y + mathMin(0, gravitySignY),
		entity.w + mathMax(0, gravitySignX),
		entity.h + mathMax(0, gravitySignY),
		"material",
		entity.id
	)

	local materialsPhysics = static.physicsMaterials
	local materialPhysics = materialsPhysics.air
	if materialEntity then
		local entityTags = materialEntity.tags

		local materials = SimulationSys.static.materials
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
function PhysicsSys.getCarryablesRecursive(entity, outCarryables, recursionDepth)
	outCarryables = outCarryables or {}

	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return outCarryables
	end

	local candidates = EntitySysFindAllRelative(
		entity, -UtilSysSign(static.physicsGravityX), -UtilSysSign(static.physicsGravityY), "physicsCarryable")
	local candidatesCount = #candidates

	for i = 1, candidatesCount do
		local candidate = candidates[i]
		local entityId = candidate.id
		if outCarryables[entityId] == nil then
			outCarryables[entityId] = candidate
			PhysicsSys.getCarryablesRecursive(candidate, outCarryables, recursionDepth + 1)
		end
	end

	return outCarryables
end
function PhysicsSys.stopX(entity)
	entity.forceX = 0
	entity.speedX = 0
	entity.overflowX = 0
end
function PhysicsSys.stopY(entity)
	entity.forceY = 0
	entity.speedY = 0
	entity.overflowY = 0
end
function PhysicsSys.tryPushX(entity, signX, recursionDepth)
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

	PhysicsSys.stopX(entity)

	if not PhysicsSys.tryMoveX(entity, signX, recursionDepth + 1, (recursionDepth ~= 1)) then
		return false
	end

	return true
end
function PhysicsSys.tryPushY(entity, signY, recursionDepth)
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

	PhysicsSys.stopY(entity)

	if not PhysicsSys.tryMoveY(entity, signY, recursionDepth + 1, (recursionDepth ~= 1)) then
		return false
	end

	return true
end
function PhysicsSys.tryMoveX(entity, moveX, recursionDepth, innerMove)
	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	local signX = UtilSysSign(moveX)
	local absMoveX = mathMin(static.physicsMaxSpeed, mathAbs(moveX))
	local moveSuccessful = true

	local curMoveX = 0
	for _ = 1, absMoveX do
		local nextMoveX = curMoveX + signX

		local obstacle = EntitySysFindRelative(entity, nextMoveX, 0, "solid")
		while obstacle and entity.physicsCanPush and PhysicsSys.tryPushX(obstacle, signX, recursionDepth + 1) do
			recursionDepth = recursionDepth + 1
			entity.forceX = entity.forceX - (signX * static.physicsPushCounterforce)
			obstacle = EntitySysFindRelative(entity, nextMoveX, 0, "solid")
			recursionDepth = recursionDepth + 1
		end
		if obstacle then
			PhysicsSys.stopX(entity)
			moveSuccessful = false
			break
		end

		curMoveX = nextMoveX
	end

	EntitySysSetBounds(entity, entity.x + curMoveX, entity.y, entity.w, entity.h)

	if curMoveX ~= 0 and entity.physicsCanCarry and not innerMove and static.physicsGravityY ~= 0 then
		for _, carryable in pairs(PhysicsSys.getCarryablesRecursive(entity, {}, recursionDepth + 1)) do
			PhysicsSys.tryMoveX(carryable, curMoveX, recursionDepth + 1, true)
		end
	end

	return moveSuccessful
end
function PhysicsSys.tryMoveY(entity, moveY, recursionDepth, innerMove)
	recursionDepth = recursionDepth or 0
	if (recursionDepth > static.physicsMaxRecursionDepth) then
		return false
	end

	local signY = UtilSysSign(moveY)
	local absMoveY = mathMin(static.physicsMaxSpeed, mathAbs(moveY))
	local moveSuccessful = true

	local curMoveY = 0
	for _ = 1, absMoveY do
		local nextMoveY = curMoveY + signY

		local obstacle = EntitySysFindRelative(entity, 0, nextMoveY, "solid")
		while obstacle and entity.physicsCanPush and PhysicsSys.tryPushY(obstacle, signY, recursionDepth + 1) do
			recursionDepth = recursionDepth + 1
			entity.forceY = entity.forceY - (signY * static.physicsPushCounterforce)
			obstacle = EntitySysFindRelative(entity, 0, nextMoveY, "solid")
		end
		if obstacle then
			PhysicsSys.stopY(entity)
			moveSuccessful = false
			break
		end

		curMoveY = nextMoveY
	end

	if curMoveY ~= 0 and entity.physicsCanCarry and not innerMove and static.physicsGravityX ~= 0 then
		for _, carryable in pairs(PhysicsSys.getCarryablesRecursive(entity, {}, recursionDepth + 1)) do
			PhysicsSys.tryMoveY(carryable, curMoveY, recursionDepth + 1, true)
		end
	end

	EntitySysSetBounds(entity, entity.x, entity.y + curMoveY, entity.w, entity.h)

	return moveSuccessful
end
function PhysicsSys.tickForces(entity)
	-- apply force to speed
	entity.speedX = entity.speedX + entity.forceX
	entity.speedY = entity.speedY + entity.forceY
	entity.forceX = 0
	entity.forceY = 0

	-- get material physics to apply
	local materialPhysics = PhysicsSys.getMaterialPhysics(entity)

	-- apply gravity to force
	entity.forceX = entity.forceX + static.physicsGravityX
	entity.forceY = entity.forceY + static.physicsGravityY

	-- apply "friction" to speed
	local speedSignX = UtilSysSign(entity.speedX)
	local speedSignY = UtilSysSign(entity.speedY)
	entity.speedX = entity.speedX - (materialPhysics.friction * UtilSysSign(entity.speedX))
	entity.speedY = entity.speedY - (materialPhysics.friction * UtilSysSign(entity.speedY))
	if UtilSysSign(entity.speedX) ~= speedSignX then
		PhysicsSys.stopX(entity)
	end
	if UtilSysSign(entity.speedY) ~= speedSignY then
		PhysicsSys.stopY(entity)
	end
end
function PhysicsSys.tickMovement(entity)
	-- clamp speed to max speed
	entity.speedX = mathMax(-static.physicsMaxSpeed, mathMin(static.physicsMaxSpeed, entity.speedX))
	entity.speedY = mathMax(-static.physicsMaxSpeed, mathMin(static.physicsMaxSpeed, entity.speedY))

	-- compute amount to move (integer values).  the fractional movement component is accumulated for subsequent ticks
	local moveX, overflowX = mathModf(entity.speedX)
	local moveY, overflowY = mathModf(entity.speedY)
	local overflowCarryX, overflowRemainderX = mathModf(overflowX + entity.overflowX)
	local overflowCarryY, overflowRemainderY = mathModf(overflowY + entity.overflowY)
	entity.overflowX = overflowRemainderX
	entity.overflowY = overflowRemainderY
	moveX = moveX + overflowCarryX
	moveY = moveY + overflowCarryY

	local signX = UtilSys.sign(moveX)
	local signY = UtilSys.sign(moveY)
	local movingX = (signX ~= 0)
	local movingY = (signY ~= 0)
	local absMoveX = math.abs(moveX)
	local absMoveY = math.abs(moveY)
	for i = 1, math.max(absMoveX, absMoveY) do
		if not movingX and not movingY then
			break
		end

		if movingX then
			movingX = PhysicsSys.tryMoveX(entity, signX) and (i < absMoveX)
		end
		if movingY then
			movingY = PhysicsSys.tryMoveY(entity, signY) and (i < absMoveY)
		end
	end
end
function PhysicsSys.tick(entity)
	PhysicsSys.tickForces(entity)
	PhysicsSys.tickMovement(entity)
end
table.insert(EntitySys.tagEvents, function(entity, tag, tagId)
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
end)
table.insert(SimulationSys.stepEvents, function()
	local world = SimulationSys.state.world
	local entities = world.entities
	local physicsEntityIds = world.tagEntities["physics"] or {}
	local physicsEntityIdsCount = #physicsEntityIds

	for i = 1, physicsEntityIdsCount do
		local entity = entities[physicsEntityIds[i]]
		PhysicsSys.tick(entity)
	end
end)

return PhysicsSys
