local EngineSys = require("src/engine/engine")
local UtilSys = EngineSys.components.UtilSys
local SimulationSys = EngineSys.components.SimulationSys
local EntitySys = EngineSys.components.EntitySys

local defaultMaterialPhysics = {
	["friction"] = 0.2,
	["moveForceStrength"] = 1,
	["jumpForceStrength"] = 1,
}
local static = SimulationSys.static
static.gravityX = 0
static.gravityY = 0.8
static.maxSpeed = 8
static.materialsPhysics = {
	["air"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {
		["moveForceStrength"] = 0.6,
	}),
	["solid"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["water"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
	["ice"] = UtilSys.tableExtend({}, defaultMaterialPhysics, {}),
}

local PhysicsSys = {}
function PhysicsSys.stopEntityX(entity)
	entity.forceX = 0
	entity.speedX = 0
	entity.overflowX = 0
end
function PhysicsSys.stopEntityY(entity)
	entity.forceY = 0
	entity.speedY = 0
	entity.overflowY = 0
end
function PhysicsSys.getEntityCollidablesArray(entity)
	local maxSpeed = SimulationSys.static.maxSpeed

	return EntitySys.findAllBounded(
		entity.x - maxSpeed - 1,
		entity.y - maxSpeed - 1,
		entity.w + maxSpeed + maxSpeed + 2,
		entity.h + maxSpeed + maxSpeed + 2,
		"material",
		entity.id
	)
end
function PhysicsSys.getEntityMaterialPhysics(entity)
	local gravitySignX = UtilSys.sign(static.gravityX)
	local gravitySignY = UtilSys.sign(static.gravityY)
	local materialEntity = EntitySys.findBounded(
		entity.x + math.min(0, gravitySignX),
		entity.y + math.min(0, gravitySignY),
		entity.w + math.max(0, gravitySignX),
		entity.h + math.max(0, gravitySignY),
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
function PhysicsSys.getEntityMaterialPhysicsInArray(entities, entity)
	local gravitySignX = UtilSys.sign(static.gravityX)
	local gravitySignY = UtilSys.sign(static.gravityY)
	local materialEntity = EntitySys.findBoundedInArray(
		entities,
		entity.x + math.min(0, gravitySignX),
		entity.y + math.min(0, gravitySignY),
		entity.w + math.max(0, gravitySignX),
		entity.h + math.max(0, gravitySignY),
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
function PhysicsSys.tickEntity(entity)
	-- fetch array of nearby collision candidates once, to reduce subsequent collision check costs
	local collidables = PhysicsSys.getEntityCollidablesArray(entity)

	-- apply force to speed
	entity.speedX = entity.speedX + entity.forceX
	entity.speedY = entity.speedY + entity.forceY
	entity.forceX = 0
	entity.forceY = 0

	-- get material physics to apply
	local materialPhysics = PhysicsSys.getEntityMaterialPhysicsInArray(collidables, entity)

	-- apply gravity to force
	entity.forceX = entity.forceX + static.gravityX
	entity.forceY = entity.forceY + static.gravityY

	-- apply "friction" to speed
	local speedSignX = UtilSys.sign(entity.speedX)
	local speedSignY = UtilSys.sign(entity.speedY)
	entity.speedX = entity.speedX - (materialPhysics.friction * UtilSys.sign(entity.speedX))
	entity.speedY = entity.speedY - (materialPhysics.friction * UtilSys.sign(entity.speedY))
	if UtilSys.sign(entity.speedX) ~= speedSignX then
		PhysicsSys.stopEntityX(entity)
	end
	if UtilSys.sign(entity.speedY) ~= speedSignY then
		PhysicsSys.stopEntityY(entity)
	end

	-- apply speed limit to speed
	entity.speedX = math.max(-static.maxSpeed, math.min(static.maxSpeed, entity.speedX))
	entity.speedY = math.max(-static.maxSpeed, math.min(static.maxSpeed, entity.speedY))

	-- compute amount to move (integer values).  the fractional movement component is accumulated for later ticks
	local moveX, overflowX = math.modf(entity.speedX)
	local moveY, overflowY = math.modf(entity.speedY)
	local overflowCarryX, overflowRemainderX = math.modf(overflowX + entity.overflowX)
	local overflowCarryY, overflowRemainderY = math.modf(overflowY + entity.overflowY)
	entity.overflowX = overflowRemainderX
	entity.overflowY = overflowRemainderY
	moveX = moveX + overflowCarryX
	moveY = moveY + overflowCarryY

	local signX = UtilSys.sign(moveX)
	local signY = UtilSys.sign(moveY)
	local absMoveX = math.abs(moveX)
	local absMoveY = math.abs(moveY)

	-- apply speed limit to movement
	absMoveX = math.min(static.maxSpeed, absMoveX)
	absMoveY = math.min(static.maxSpeed, absMoveY)

	-- move horizontally
	for _ = 1, absMoveX do
		if EntitySys.findRelativeInArray(collidables, entity, signX, 0, "solid", entity.id) then
			PhysicsSys.stopEntityX(entity)
			break
		else
			EntitySys.setBounds(entity, entity.x + signX, entity.y, entity.w, entity.h)
		end
	end

	-- move vertically
	for _ = 1, absMoveY do
		if EntitySys.findRelativeInArray(collidables, entity, 0, signY, "solid", entity.id) then
			PhysicsSys.stopEntityY(entity)
			break
		else
			EntitySys.setBounds(entity, entity.x, entity.y + signY, entity.w, entity.h)
		end
	end
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
	end
end)
table.insert(SimulationSys.stepEvents, function()
	local world = SimulationSys.state.world
	local entities = world.entities
	local physicsEntityIds = world.tagEntities["physics"] or {}
	local physicsEntityIdsCount = #physicsEntityIds

	for i = 1, physicsEntityIdsCount do
		local entity = entities[physicsEntityIds[i]]
		PhysicsSys.tickEntity(entity)
	end
end)

return PhysicsSys
