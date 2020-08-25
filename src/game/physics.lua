local UtilSys = require("src/engine/util")
local SimulationSys = require("src/engine/simulation")
local EntitySys = require("src/engine/entity")

local mathAbs = math.abs
local mathMin = math.min
local mathMax = math.max
local mathModf = math.modf
local UtilSysSign = UtilSys.sign
local EntitySysFindRelative = EntitySys.findRelative
local EntitySysSetBounds = EntitySys.setBounds

local static = SimulationSys.static


local defaultMaterialPhysics = {
	["friction"] = 0.2,
	["moveForceStrength"] = 1,
	["jumpForceStrength"] = 1,
}
static.gravityX = 0
static.gravityY = 0.8
static.maxSpeed = 8
static.moveMaxStackDepth = 50
static.materialsPhysics = {
	["air"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {
		["moveForceStrength"] = 0.6,
	}),
	["solid"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["water"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["ice"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["death"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
}

local PhysicsSys = {}
function PhysicsSys.getMaterialPhysics(entity)
	local gravitySignX = UtilSysSign(static.gravityX)
	local gravitySignY = UtilSysSign(static.gravityY)
	local materialEntity = EntitySys.findBounded(
		entity.x + mathMin(0, gravitySignX),
		entity.y + mathMin(0, gravitySignY),
		entity.w + mathMax(0, gravitySignX),
		entity.h + mathMax(0, gravitySignY),
		"material",
		entity.id
	)

	local materialsPhysics = static.materialsPhysics
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
function PhysicsSys.tryPushX(entity, signX, stackDepth)
	stackDepth = stackDepth or 0
	if (stackDepth > static.moveMaxStackDepth) then
		return false
	end

	if not entity.tags.physics then
		return false
	end

	if not entity.physicsCanBePushed then
		return false
	end

	PhysicsSys.stopX(entity)

	if not PhysicsSys.tryMoveX(entity, signX, stackDepth + 1) then
		return false
	end

	return true
end
function PhysicsSys.tryPushY(entity, signY, stackDepth)
	stackDepth = stackDepth or 0
	if (stackDepth > static.moveMaxStackDepth) then
		return false
	end

	if not entity.tags.physics then
		return false
	end

	if not entity.physicsCanBePushed then
		return false
	end

	PhysicsSys.stopY(entity)

	if not PhysicsSys.tryMoveY(entity, signY, stackDepth + 1) then
		return false
	end

	return true
end
function PhysicsSys.tryMoveX(entity, moveX, stackDepth)
	stackDepth = stackDepth or 0
	if (stackDepth > static.moveMaxStackDepth) then
		return false
	end

	local signX = UtilSysSign(moveX)
	local absMoveX = mathMin(static.maxSpeed, mathAbs(moveX))
	local moveSuccessful = true

	local curMoveX = 0
	for _ = 1, absMoveX do
		local nextMoveX = curMoveX + signX

		local obstacle = EntitySysFindRelative(entity, nextMoveX, 0, "solid")
		while obstacle and PhysicsSys.tryPushX(obstacle, signX, stackDepth + 1) do
			entity.forceX = entity.forceX - (signX * 0.2)
			obstacle = EntitySysFindRelative(entity, nextMoveX, 0, "solid")
		end
		if obstacle then
			PhysicsSys.stopX(entity)
			moveSuccessful = false
			break
		end

		curMoveX = nextMoveX
	end

	EntitySysSetBounds(entity, entity.x + curMoveX, entity.y, entity.w, entity.h)

	return moveSuccessful
end
function PhysicsSys.tryMoveY(entity, moveY, stackDepth)
	stackDepth = stackDepth or 0
	if (stackDepth > static.moveMaxStackDepth) then
		return false
	end

	local signY = UtilSysSign(moveY)
	local absMoveY = mathMin(static.maxSpeed, mathAbs(moveY))
	local moveSuccessful = true

	local curMoveY = 0
	for _ = 1, absMoveY do
		local nextMoveY = curMoveY + signY

		local obstacle = EntitySysFindRelative(entity, 0, nextMoveY, "solid")
		while obstacle and PhysicsSys.tryPushY(obstacle, signY, stackDepth + 1) do
			entity.forceY = entity.forceY - (signY * 0.2)
			obstacle = EntitySysFindRelative(entity, 0, nextMoveY, "solid")
		end
		if obstacle then
			PhysicsSys.stopY(entity)
			moveSuccessful = false
			break
		end

		curMoveY = nextMoveY
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
	entity.forceX = entity.forceX + static.gravityX
	entity.forceY = entity.forceY + static.gravityY

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
	entity.speedX = mathMax(-static.maxSpeed, mathMin(static.maxSpeed, entity.speedX))
	entity.speedY = mathMax(-static.maxSpeed, mathMin(static.maxSpeed, entity.speedY))

	-- compute amount to move (integer values).  the fractional movement component is accumulated for subsequent ticks
	local moveX, overflowX = mathModf(entity.speedX)
	local moveY, overflowY = mathModf(entity.speedY)
	local overflowCarryX, overflowRemainderX = mathModf(overflowX + entity.overflowX)
	local overflowCarryY, overflowRemainderY = mathModf(overflowY + entity.overflowY)
	entity.overflowX = overflowRemainderX
	entity.overflowY = overflowRemainderY
	moveX = moveX + overflowCarryX
	moveY = moveY + overflowCarryY

	PhysicsSys.tryMoveX(entity, moveX)
	PhysicsSys.tryMoveY(entity, moveY)
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
		entity.physicsCanBePushed = entity.physicsCanBePushed or false

		entity.physicsCanCarry = entity.physicsCanCarry or false
		entity.physicsCanBeCarried = entity.physicsCanBeCarried or false
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
