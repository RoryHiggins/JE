local log = require("engine/util/log")
local util = require("engine/util/util")

local FLOAT_EPSILON = 1.19e-07
local utilRectCollides = util.rectCollides
local mathFloor = math.floor
local stringFormat = string.format

local Entity = {}
Entity.SYSTEM_NAME = "entity"
Entity.ENTITY_CHUNK_SIZE = 64
function Entity:setBounds(entity, x, y, w, h)
	local oldEntityX = entity.x
	local oldEntityY = entity.y

	local oldEntityW = entity.w
	local oldEntityH = entity.h

	-- optimization: early return if bounds are unchanged, as this call is expensive
	if ((oldEntityX == x) and (oldEntityY == y)
		and (oldEntityW == w) and (oldEntityH == h)) then
		return
	end

	-- cannot move a destroyed entity or non-entity
	local entityId = entity.id
	if entityId == nil then
		return
	end

	local entityChunkSize = self.ENTITY_CHUNK_SIZE
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

	local world = self.simulation.state.world
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
				local chunkKey = stringFormat("%d,%d", oldChunkX, oldChunkY)
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
				local chunkKey = stringFormat("%d,%d", chunkX, chunkY)
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
function Entity:setPos(entity, x, y)
	self:setBounds(entity, x, y, entity.w, entity.h)
end
function Entity:setSize(entity, w, h)
	self:setBounds(entity, entity.x, entity.y, w, h)
end
function Entity:movePos(entity, offsetX, offsetY)
	self:setBounds(entity, entity.x + offsetX, entity.y + offsetY, entity.w, entity.h)
end
function Entity:tag(entity, tag)
	local entityTags = entity.tags
	if entityTags[tag] ~= nil then
		return
	end

	local worldTagEntities = self.simulation.state.world.tagEntities
	local tagEntities = worldTagEntities[tag]

	if tagEntities == nil then
		tagEntities = {}
		worldTagEntities[tag] = tagEntities
	end

	local entityId = entity.id
	local tagId = #tagEntities + 1
	tagEntities[tagId] = entityId

	entityTags[tag] = tagId

	self.simulation:broadcast("onEntityTag", false, entity, tag, tagId)
end
function Entity:untag(entity, tag)
	local entityTags = entity.tags
	local tagId = entityTags[tag]
	if tagId == nil then
		return
	end

	local world = self.simulation.state.world
	local worldTagEntities = world.tagEntities
	local tagEntities = worldTagEntities[tag]
	local tagsCount = #tagEntities

	local swapTag = tagEntities[tagsCount]
	local swapEntity = world.entities[swapTag]
	tagEntities[tagId] = swapEntity.id
	swapEntity.tags[tag] = tagId

	tagEntities[tagsCount] = nil
	entityTags[tag] = nil

	self.simulation:broadcast("onEntityTag", false, entity, tag, nil)
end
function Entity:find(tag, getAll)
	local world = self.simulation.state.world

	local worldEntities = world.entities
	local tagEntities = world.tagEntities[tag] or {}
	if not getAll then
		local entityId = tagEntities[1]
		if entityId == nil then
			return nil
		end
		return worldEntities[entityId]
	end

	local tagEntitiesCount = #tagEntities
	local results = {}

	for i = 1, tagEntitiesCount do
		results[#results + 1] = worldEntities[tagEntities[i]]
	end

	return results
end
function Entity:findAll(tag)
	return self:find(tag, true)
end
function Entity:findBounded(x, y, w, h, filterTag, filterOutEntityId, getAll)
	local seenResults = nil
	local results = nil
	if getAll then
		results = {}
		seenResults = {}
	end

	local entityChunkSize = self.ENTITY_CHUNK_SIZE
	local chunkX1 = mathFloor(x / entityChunkSize)
	local chunkY1 = mathFloor(y / entityChunkSize)
	local chunkX2 = mathFloor((x + w - FLOAT_EPSILON) / entityChunkSize)
	local chunkY2 = mathFloor((y + h - FLOAT_EPSILON) / entityChunkSize)

	local world = self.simulation.state.world
	local worldChunks = world.chunkEntities
	local entities = world.entities
	for chunkY = chunkY1, chunkY2 do
		for chunkX = chunkX1, chunkX2 do
			local chunkKey = stringFormat("%d,%d", chunkX, chunkY)
			local chunkEntities = worldChunks[chunkKey]
			if chunkEntities == nil then
				chunkEntities = {}
			end

			local chunkEntitiesCount = #chunkEntities
			for i = 1, chunkEntitiesCount do
				local entityId = chunkEntities[i]
				if (filterOutEntityId == nil) or (entityId ~= filterOutEntityId) then
					local entity = entities[entityId]
					if utilRectCollides(x, y, w, h, entity.x, entity.y, entity.w, entity.h) then
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
function Entity:findAllBounded(x, y, w, h, filterTag, filterOutEntityId)
	return self:findBounded(x, y, w, h, filterTag, filterOutEntityId, true)
end
function Entity:findRelative(entity, offsetX, offsetY, filterTag, getAll)
	local relativeX = entity.x + (offsetX or 0)
	local relativeY = entity.y + (offsetY or 0)

	return self:findBounded(relativeX, relativeY, entity.w, entity.h, filterTag, entity.id, getAll)
end
function Entity:findAllRelative(entity, offsetX, offsetY, filterTag)
	return self:findRelative(entity, offsetX, offsetY, filterTag, true)
end
function Entity:destroy(entity)
	if entity.destroyed then
		return
	end

	self.simulation:broadcast("onEntityDestroy", false, entity)

	local entityTags = entity.tags
	for tag, _ in pairs(entityTags) do
		self:untag(entity, tag)
	end

	self:setBounds(entity, 0, 0, 0, 0)

	local entityId = entity.id
	for key, _ in pairs(entity) do
		entity[key] = nil
	end

	entity.destroyed = true

	local destroyedEntities = self.simulation.state.world.destroyedEntities
	destroyedEntities[#destroyedEntities + 1] = entityId
end
function Entity:create()
	local world = self.simulation.state.world
	local entities = world.entities

	local destroyedEntities = world.destroyedEntities
	local destroyedEntitiesCount = #destroyedEntities

	local entityId
	if destroyedEntitiesCount > 0 then
		entityId = destroyedEntities[destroyedEntitiesCount]
		destroyedEntities[destroyedEntitiesCount] = nil
	else
		entityId = #entities + 1
	end

	local entity = {
		["id"] = entityId,
		["x"] = 0,
		["y"] = 0,
		["z"] = 0,
		["w"] = 0,
		["h"] = 0,
		["chunks"] = {},
		["tags"] = {},
	}

	entities[entityId] = entity

	return entity
end
function Entity:onInit(simulation)
	self.simulation = simulation
end
function Entity:onWorldInit()
	local world = self.simulation.state.world
	world.entities = {}
	world.tagEntities = {}
	world.chunkEntities = {}
	world.destroyedEntities = {}
end
function Entity:onRunTests()
	self.simulation:worldInit()

	local entityChunkSizeBackup = self.ENTITY_CHUNK_SIZE
	self.ENTITY_CHUNK_SIZE = 64  -- test values are hard-coded to test this chunk size

	local world = self.simulation.state.world
	if world.entities == nil then
		log.error("entities object was not created during World.create()")
	end
	if world.tagEntities == nil then
		log.error("tagEntities object was not created during World.create()")
	end
	if world.chunkEntities == nil then
		log.error("chunkEntities object was not created during World.create()")
	end

	local entity = self:create()
	local entity2 = self:create()
	log.assert(entity.id ~= entity2.id)
	log.assert(world.entities[entity.id] == entity)
	log.assert(world.entities[entity2.id] == entity2)

	log.assert((entity.x == 0) and (entity.y == 0) and (entity.w == 0) and (entity.h == 0))
	log.assert(util.setEquals(util.tableGetKeys(entity.chunks), {}))
	self:tag(entity, "red")
	log.assert(self:findBounded(0, 0, 100, 100, "red") == nil)
	log.assert(#self:findAllBounded(0, 0, 100, 100, "red") == 0)

	self:setBounds(entity, 64, 96, 16, 32)
	log.assert(entity.x == 64 and entity.y == 96 and entity.w == 16 and entity.h == 32)
	log.assert(util.setEquals(util.tableGetKeys(entity.chunks), {"1,1"}))
	log.assert(world.chunkEntities["1,1"][entity.chunks["1,1"]] == entity.id)
	log.assert(#self:findAllBounded(0, 0, 100, 100, "red") == 1)
	log.assert(self:findBounded(0, 0, 100, 100, "red"))
	log.assert(self:findBounded(0, 0, 64, 96, "red") == nil)
	log.assert(self:findBounded(0, 0, 65, 97, "red"))
	log.assert(self:findBounded(80, 128, 1, 1, "red") == nil)
	log.assert(self:findBounded(79, 127, 1, 1, "red"))

	log.assert(self:findRelative(entity, 0, 0, "red") == nil)
	log.assert(#self:findAllRelative(entity, 0, 0, "red") == 0)

	self:setPos(entity, 32, 32)
	log.assert(entity.x == 32)
	log.assert(entity.y == 32)

	self:setBounds(entity, 0, 0, 0, 0)
	log.assert(util.setEquals(util.tableGetKeys(entity.chunks), {}))

	self:untag(entity, "red")
	self:tag(entity, "red")
	log.assert(util.setEquals(util.tableGetKeys(entity.tags), {"red"}))

	self:tag(entity, "green")
	log.assert(util.setEquals(util.tableGetKeys(entity.tags), {"red", "green"}))

	log.assert(self:find("red") == entity)
	log.assert(util.setEquals(self:findAll("red"), {entity}))

	self:tag(entity2, "red")
	log.assert(util.setEquals(self:findAll("red"), {entity, entity2}))

	self:destroy(entity)
	log.assert(entity.destroyed)
	log.assert(util.setEquals(self:findAll("red"), {entity2}))

	self:destroy(entity2)
	log.assert(entity.destroyed)
	log.assert(util.setEquals(self:findAll("red"), {}))

	local entities = {}
	local numEntities = 5
	log.assert(#self:findAllBounded(0, 0, 100, 100, "blue") == 0)
	for i = 1, numEntities do
		entity = self:create()
		self:setBounds(entity, i, i, 1, 1)
		self:tag(entity, tostring(i))
		self:tag(entity, "blue")

		log.assert(self:find(tostring(i)) == entity)

		entities[#entities + 1] = entity
	end

	log.assert(util.setEquals(self:findAll("blue"), entities))
	log.assert(#self:findAllBounded(0, 0, 100, 100, "blue") == numEntities)

	for _, entityToDestroy in ipairs(entities) do
		self:destroy(entityToDestroy)
	end

	log.assert(util.setEquals(self:findAll("blue"), {}))

	self.ENTITY_CHUNK_SIZE = entityChunkSizeBackup
end

return Entity
