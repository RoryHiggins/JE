local json = require("./lib/json")
local UtilSys = require("src/engine/util")
local SimulationSys = require("src/engine/simulation")
local WorldSys = require("src/engine/world")

local FLOAT_EPSILON = 1.19e-07

local EntitySys = {}
EntitySys.destroyEvents = {}
EntitySys.tagEvents = {}
EntitySys.ENTITY_CHUNK_SIZE = 64
function EntitySys.setBounds(entity, x, y, w, h)
	local oldEntityX = entity.x
	local oldEntityY = entity.y

	local oldEntityW = entity.w
	local oldEntityH = entity.h

	local entityId = entity.id
	if entityId == nil then
		return
	end

	local entityChunkSize = EntitySys.ENTITY_CHUNK_SIZE
	local mathFloor = math.floor
	local oldChunkX1 = mathFloor(oldEntityX / entityChunkSize)
	local oldChunkY1 = mathFloor(oldEntityY / entityChunkSize)
	local oldChunkX2 = mathFloor((oldEntityX + oldEntityW - FLOAT_EPSILON) / entityChunkSize)
	local oldChunkY2 = mathFloor((oldEntityY + oldEntityH - FLOAT_EPSILON) / entityChunkSize)
	if oldEntityW <= 0 then
		oldChunkX2 = oldChunkX1 - 1
	end
	if oldEntityH <= 0 then
		oldChunkY2 = oldChunkY1 - 1
	end

	local chunkX1 = mathFloor(x / entityChunkSize)
	local chunkY1 = mathFloor(y / entityChunkSize)
	local chunkX2 = mathFloor((x + w - FLOAT_EPSILON) / entityChunkSize)
	local chunkY2 = mathFloor((y + h - FLOAT_EPSILON) / entityChunkSize)
	if w <= 0 then
		chunkX2 = chunkX1 - 1
	end
	if h <= 0 then
		chunkY2 = chunkY1 - 1
	end

	local world = SimulationSys.state.world
	local worldEntities = world.entities
	local entityChunks = entity.chunks
	local worldChunks = world.chunkEntities

	-- remove entity from chunks outside new bounds
	for oldChunkY = oldChunkY1, oldChunkY2 do
		for oldChunkX = oldChunkX1, oldChunkX2 do
			local outsideNewBounds = (
				(oldChunkX < chunkX1)
				or (oldChunkY < chunkY1)
				or (oldChunkX > chunkX2)
				or (oldChunkY > chunkY2)
			)
			if outsideNewBounds then
				local chunkKey = string.format("%d,%d", oldChunkX, oldChunkY)
				local chunkId = entityChunks[chunkKey]

				-- swap and pop entity from chunks array
				local chunk = worldChunks[chunkKey]
				local swapChunkId = #chunk
				local swapEntityId = chunk[swapChunkId]
				local swapEntityChunks = worldEntities[swapEntityId].chunks
				chunk[chunkId] = swapEntityId
				swapEntityChunks[chunkKey] = chunkId

				chunk[swapChunkId] = nil
				entityChunks[chunkKey] = nil
			end
		end
	end

	-- add entity to chunks outside old bounds
	for chunkY = chunkY1, chunkY2 do
		for chunkX = chunkX1, chunkX2 do
			local outsideOldBounds = (
				(chunkX < oldChunkX1)
				or (chunkY < oldChunkY1)
				or (chunkX > oldChunkX2)
				or (chunkY > oldChunkY2)
			)
			if outsideOldBounds then
				local chunkKey = string.format("%d,%d", chunkX, chunkY)
				local chunk = worldChunks[chunkKey]
				if chunk == nil then
					chunk = {}
					worldChunks[chunkKey] = chunk
				end

				local chunkId = #chunk + 1
				chunk[chunkId] = entityId
				entityChunks[chunkKey] = chunkId
			end
		end
	end

	entity.x = x
	entity.y = y
	entity.w = w
	entity.h = h
end
function EntitySys.setPos(entity, x, y)
	EntitySys.setBounds(entity, x, y, entity.w, entity.h)
end
function EntitySys.tag(entity, tag)
	local entityTags = entity.tags
	if entityTags[tag] ~= nil then
		return
	end

	local worldTagEntities = SimulationSys.state.world.tagEntities
	local tagEntities = worldTagEntities[tag]

	if tagEntities == nil then
		tagEntities = {}
		worldTagEntities[tag] = tagEntities
	end

	local entityId = entity.id
	local tagId = #tagEntities + 1
	tagEntities[tagId] = entityId

	entityTags[tag] = tagId

	local events = EntitySys.tagEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](entity, tag, tagId)
	end
end
function EntitySys.untag(entity, tag)
	local entityTags = entity.tags
	local tagId = entityTags[tag]
	if tagId == nil then
		return
	end

	local world = SimulationSys.state.world
	local worldTagEntities = world.tagEntities
	local tagEntities = worldTagEntities[tag]
	local tagsCount = #tagEntities

	local swapTag = tagEntities[tagsCount]
	local swapEntity = world.entities[swapTag]
	tagEntities[tagId] = swapEntity.id
	swapEntity.tags[tag] = tagId

	tagEntities[tagsCount] = nil
	entityTags[tag] = nil

	local events = EntitySys.tagEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](entity, tag, nil)
	end
end
function EntitySys.find(tag)
	local world = SimulationSys.state.world
	local tagEntities = world.tagEntities[tag]
	if tagEntities == nil then
		return nil
	end

	local entityId = tagEntities[1]
	if entityId == nil then
		return nil
	end

	return world.entities[entityId]
end
function EntitySys.findAll(tag)
	local world = SimulationSys.state.world
	local worldEntities = world.entities

	local entities = {}
	for _, entityId in pairs(SimulationSys.state.world.tagEntities[tag] or {}) do
		entities[#entities + 1] = worldEntities[entityId]
	end

	return entities
end
function EntitySys.findBounded(x, y, w, h, filterTag, filterOutEntityId, getAll)
	local seenResults = nil
	local results = nil
	if getAll then
		results = {}
		seenResults = {}
	end

	local entityChunkSize = EntitySys.ENTITY_CHUNK_SIZE
	local mathFloor = math.floor
	local chunkX1 = mathFloor(x / entityChunkSize)
	local chunkY1 = mathFloor(y / entityChunkSize)
	local chunkX2 = mathFloor((x + w - FLOAT_EPSILON) / entityChunkSize)
	local chunkY2 = mathFloor((y + h - FLOAT_EPSILON) / entityChunkSize)

	local world = SimulationSys.state.world
	local worldChunks = world.chunkEntities
	local entities = world.entities
	for chunkY = chunkY1, chunkY2 do
		for chunkX = chunkX1, chunkX2 do
			local chunkKey = string.format("%d,%d", chunkX, chunkY)
			local chunkEntities = worldChunks[chunkKey]
			if chunkEntities == nil then
				chunkEntities = {}
			end

			local chunkEntitiesCount = #chunkEntities
			for i = 1, chunkEntitiesCount do
				local entityId = chunkEntities[i]
				if (filterOutEntityId == nil) or (entityId ~= filterOutEntityId) then
					local entity = entities[entityId]
					if UtilSys.rectCollides(x, y, w, h, entity.x, entity.y, entity.w, entity.h) then
						if (filterTag == nil) or (entity.tags[filterTag] ~= nil) then
							if not getAll then
								return entity
							end

							local entityIdString = tostring(entityId)
							if not seenResults[entityIdString] then
								results[#results + 1] = entity
								seenResults[entityIdString] = true
							end
						end
					end
				end
			end
		end
	end

	return results
end
function EntitySys.findAllBounded(x, y, w, h, filterTag, filterOutEntityId)
	return EntitySys.findBounded(x, y, w, h, filterTag, filterOutEntityId, true)
end
function EntitySys.findRelative(entity, signX, signY, filterTag)
	local relativeX = entity.x + (signX or 0)
	local relativeY = entity.y + (signY or 0)

	return EntitySys.findBounded(relativeX, relativeY, entity.w, entity.h, filterTag, entity.id)
end
function EntitySys.findBoundedInArray(entities, x, y, w, h, filterTag, filterOutEntityId, getAll)
	local seenResults = nil
	local results = nil
	if getAll then
		results = {}
		seenResults = {}
	end

	local entitiesCount = #entities
	for i = 1, entitiesCount do
		local entityId = entities[i]
		if (filterOutEntityId == nil) or (entityId ~= filterOutEntityId) then
			local entity = entities[i]
			if (((filterTag == nil) or (entity.tags[filterTag] ~= nil))
				and UtilSys.rectCollides(x, y, w, h, entity.x, entity.y, entity.w, entity.h)) then
				-- if filterOutEntityId == 1 then
				-- 	print("Collision: ", entity.id, "@", entity.x, entity.y, entity.w, entity.h, UtilSys.toComparable(entity.tags),
				-- 		  " against", filterOutEntityId, "@", x, y, w, h)
				-- end
				if not getAll then
					return entity
				end

				local entityIdString = tostring(entityId)
				if not seenResults[entityIdString] then
					results[#results + 1] = entity
					seenResults[entityIdString] = true
				end
			end
		end
	end

	return results
end
function EntitySys.findAllBoundedInArray(entities, x, y, w, h, filterTag, filterOutEntityId)
	return EntitySys.findBoundedInArray(entities, x, y, w, h, filterTag, filterOutEntityId, --[[getAll--]] true)
end
function EntitySys.findRelativeInArray(entities, entity, signX, signY, filterTag)
	local relativeX = entity.x + signX
	local relativeY = entity.y + signY

	return EntitySys.findBoundedInArray(entities,  relativeX, relativeY, entity.w, entity.h, filterTag, entity.id)
end
function EntitySys.destroy(entity)
	if entity.destroyed then
		return
	end

	local events = EntitySys.destroyEvents
	local eventsCount = #events
	for i = 1, eventsCount do
		events[i](entity)
	end

	local entityTags = entity.tags
	for tag, _ in pairs(entityTags) do
		EntitySys.untag(entity, tag)
	end

	EntitySys.setBounds(entity, 0, 0, 0, 0)

	local entityId = entity.id
	for key, _ in pairs(entity) do
		entity[key] = nil
	end

	entity.destroyed = true

	local destroyedEntities = SimulationSys.state.world.destroyedEntities
	destroyedEntities[#destroyedEntities + 1] = entityId
end
function EntitySys.create(template)
	local world = SimulationSys.state.world
	local entities = world.entities
	local entitiesCount = #entities

	local destroyedEntities = world.destroyedEntities
	local destroyedEntitiesCount = #destroyedEntities

	local entityId
	if destroyedEntitiesCount > 0 then
		entityId = destroyedEntities[destroyedEntitiesCount]
		destroyedEntities[destroyedEntitiesCount] = nil
	else
		entityId = entitiesCount + 1
	end

	local entity
	if template ~= nil then
		entity = json.decode(json.encode(template))
	else
		entity = {}
	end

	entity.id = entityId
	entity.x = 0
	entity.y = 0
	entity.w = 0
	entity.h = 0
	entity.chunks = {}
	entity.tags = {}

	if template ~= nil then
		EntitySys.setBounds(entity, template.x or 0, template.y or 0, template.w or 0, template.h or 0)

		local templateTags = template.tags
		if templateTags ~= nil then
			for tag, _ in pairs(templateTags) do
				EntitySys.tag(entity, tag)
			end
		end
	end

	world.entities[entityId] = entity

	return entity
end
function EntitySys.runTests()
	SimulationSys.create()

	local entityChunkSizeBackup = EntitySys.ENTITY_CHUNK_SIZE
	EntitySys.ENTITY_CHUNK_SIZE = 64  -- test values are hard-coded to test this chunk size

	local world = SimulationSys.state.world
	if world.entities == nil then
		UtilSys.err("EntitySys.runTests(): entities object was not created during WorldSys.create()")
	end
	if world.tagEntities == nil then
		UtilSys.err("EntitySys.runTests(): tagEntities object was not created during WorldSys.create()")
	end
	if world.chunkEntities == nil then
		UtilSys.err("EntitySys.runTests(): chunkEntities object was not created during WorldSys.create()")
	end

	local entity = EntitySys.create()
	local entity2 = EntitySys.create()
	assert(entity.id ~= entity2.id)
	assert(world.entities[entity.id] == entity)
	assert(world.entities[entity2.id] == entity2)

	assert((entity.x == 0) and (entity.y == 0) and (entity.w == 0) and (entity.h == 0))
	assert(UtilSys.setEquality(UtilSys.getKeys(entity.chunks), {}))
	EntitySys.tag(entity, "yolo")
	assert(EntitySys.findBounded(0, 0, 100, 100, "yolo") == nil)
	assert(#EntitySys.findAllBounded(0, 0, 100, 100, "yolo") == 0)

	EntitySys.setBounds(entity, 64, 96, 16, 32)
	assert(entity.x == 64 and entity.y == 96 and entity.w == 16 and entity.h == 32)
	assert(UtilSys.setEquality(UtilSys.getKeys(entity.chunks), {"1,1"}))
	assert(world.chunkEntities["1,1"][entity.chunks["1,1"]] == entity.id)
	assert(#EntitySys.findAllBounded(0, 0, 100, 100, "yolo") == 1)
	assert(EntitySys.findBounded(0, 0, 100, 100, "yolo"))
	assert(EntitySys.findBounded(0, 0, 64, 96, "yolo") == nil)
	assert(EntitySys.findBounded(0, 0, 65, 97, "yolo"))
	assert(EntitySys.findBounded(80, 128, 1, 1, "yolo") == nil)
	assert(EntitySys.findBounded(79, 127, 1, 1, "yolo"))

	assert(EntitySys.findRelative(entity) == nil)

	EntitySys.setBounds(entity, 0, 0, 0, 0)
	assert(UtilSys.setEquality(UtilSys.getKeys(entity.chunks), {}))

	EntitySys.untag(entity, "yolo")
	EntitySys.tag(entity, "yolo")
	assert(UtilSys.setEquality(UtilSys.getKeys(entity.tags), {"yolo"}))

	EntitySys.tag(entity, "swag")
	assert(UtilSys.setEquality(UtilSys.getKeys(entity.tags), {"yolo", "swag"}))

	assert(EntitySys.find("yolo") == entity)
	assert(UtilSys.setEquality(EntitySys.findAll("yolo"), {entity}))

	EntitySys.tag(entity2, "yolo")
	assert(UtilSys.setEquality(EntitySys.findAll("yolo"), {entity, entity2}))

	EntitySys.destroy(entity)
	assert(entity.destroyed)
	assert(UtilSys.setEquality(EntitySys.findAll("yolo"), {entity2}))

	EntitySys.destroy(entity2)
	assert(entity.destroyed)
	assert(UtilSys.setEquality(EntitySys.findAll("yolo"), {}))

	local entities = {}
	local numEntities = 5
	assert(#EntitySys.findAllBounded(0, 0, 100, 100, "ysg") == 0)
	for i = 1, numEntities do
		entity = EntitySys.create()
		EntitySys.setBounds(entity, i, i, 1, 1)
		EntitySys.tag(entity, tostring(i))
		EntitySys.tag(entity, "ysg")

		assert(EntitySys.find(tostring(i)) == entity)

		entities[#entities + 1] = entity
	end

	assert(UtilSys.setEquality(EntitySys.findAll("ysg"), entities))
	assert(UtilSys.setEquality(EntitySys.findAllBoundedInArray(EntitySys.findAll("ysg"), 0, 0, 101, 101, "ysg"),
							EntitySys.findAll("ysg")))
	assert(#EntitySys.findAllBounded(0, 0, 100, 100, "ysg") == numEntities)

	for _, entityToDestroy in ipairs(entities) do
		EntitySys.destroy(entityToDestroy)
	end

	assert(UtilSys.setEquality(EntitySys.findAll("ysg"), {}))

	EntitySys.ENTITY_CHUNK_SIZE = entityChunkSizeBackup
end
table.insert(WorldSys.createEvents, function()
	local world = SimulationSys.state.world
	world.entities = {}
	world.tagEntities = {}
	world.chunkEntities = {}
	world.destroyedEntities = {}
end)

return EntitySys
