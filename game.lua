-- ---------------- Engine ----------------
-- Dependencies
	local json = require("json")

-- Utilities (TODO split up)
	local UtilSys = {}
	function UtilSys.log(format, ...)
		local callee_info = debug.getinfo(2, "Sl")
		print(string.format("[LOG %s:%d] "..format, callee_info.short_src, callee_info.currentline, ...))
	end
	function UtilSys.err(format, ...)
		local callee_info = debug.getinfo(2, "Sl")
		print(string.format("[ERR %s:%d] "..format, callee_info.short_src, callee_info.currentline, ...))
	end
	function UtilSys.noop()
	end
	function UtilSys.getKeys(input)
		local keys = {}

		for key, _ in pairs(input) do
			keys[#keys + 1] = key
		end
		table.sort(keys)

		return keys
	end
	function UtilSys.getEscapedString(input)
		return (
			input
			:gsub('\\', '\\\\')
			:gsub('\n', '\\n')
			:gsub('\r', '\\r')
			:gsub('\"', '\\"')
			:gsub("[^%w%p%s]", "?")
		)
	end
	function UtilSys.toComparable(input, stack)
		local inputType = type(input)

		if inputType == "number" then
			return tostring(input)
		elseif inputType == "string" then
			return "\""..UtilSys.getEscapedString(input).."\""
		elseif inputType == "table" then
			stack = stack or {}
			for _, seenInput in pairs(stack) do
				if tostring(input) == tostring(seenInput) then
					return "\"<unserializable table recursion>\""
				end
			end

			stack[#stack + 1] = input

			local keys = UtilSys.getKeys(input)

			local fieldStrings = {}
			for _, key in ipairs(keys) do
				local val = input[key]
				local fieldString = string.format("%s: %s", UtilSys.toComparable(key), UtilSys.toComparable(val, stack))
				fieldStrings[#fieldStrings + 1] = fieldString
			end

			return "{"..table.concat(fieldStrings, ", ").."}"
		elseif inputType == "nil" then
			return "null"
		elseif inputType == "function" then
			return UtilSys.toComparable(tostring(input))
		else
			return tostring(input)
		end
	end
	function UtilSys.getValues(input)
		local values = {}

		for _, value in pairs(input) do
			values[#values + 1] = value
		end
		table.sort(values, function (a, b) return UtilSys.toComparable(a) < UtilSys.toComparable(b) end)

		local duplicateIndices = {}

		for i = 1, #values do
			local value = values[i]
			for j = 1, (i - 1) do
				if UtilSys.toComparable(value) == UtilSys.toComparable(values[j]) then
					duplicateIndices[#duplicateIndices + 1] = i
					break
				end
			end
		end

		for _, duplicateIndex in ipairs(duplicateIndices) do
			table.remove(values, duplicateIndex)
		end

		return values
	end
	function UtilSys.setEquality(a, b)
		return UtilSys.toComparable(UtilSys.getValues(a)) == UtilSys.toComparable(UtilSys.getValues(b))
	end
	function UtilSys.runTests()
		UtilSys.noop()
		assert(UtilSys.getKeys({["a"] = 1})[1] == "a")
		assert(UtilSys.getEscapedString("\n\"\0") == "\\n\\\"?")
		assert(UtilSys.toComparable(nil) == UtilSys.toComparable(nil))
		assert(UtilSys.toComparable(nil) ~= UtilSys.toComparable(1))
		assert(UtilSys.toComparable(1) == UtilSys.toComparable(1))
		assert(UtilSys.toComparable(1) ~= UtilSys.toComparable(2))
		assert(UtilSys.toComparable("a") == UtilSys.toComparable("a"))
		assert(UtilSys.toComparable("a") ~= UtilSys.toComparable("b"))
		assert(UtilSys.toComparable({["a"] = 1, ["b"] = 2}) == UtilSys.toComparable({["a"] = 1, ["b"] = 2}))
		assert(UtilSys.toComparable({["a"] = 1, ["b"] = 2}) ~= UtilSys.toComparable({["a"] = 1, ["b"] = 2, ["c"] = 3}))
		assert(UtilSys.toComparable({["a"] = 1, ["b"] = 2}) ~= UtilSys.toComparable({["a"] = 2, ["b"] = 2}))
		assert(UtilSys.toComparable(_G) == UtilSys.toComparable(_G))
		assert(UtilSys.toComparable(UtilSys.getValues({1, 2, 3})) == UtilSys.toComparable({1, 2, 3}))
		assert(UtilSys.toComparable(UtilSys.getValues({["a"] = 1, ["b"] = 2, ["c"] = 3})) == UtilSys.toComparable({1, 2, 3}))
		assert(UtilSys.setEquality({}, {}))
		assert(UtilSys.setEquality({1, 2}, {2, 1, 1}))
		assert(UtilSys.setEquality({1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}, {1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}))
		assert(not UtilSys.setEquality({["c"] = {["d"] = 1}}, {["c"] = {["d"] = 2}}))
		assert(not UtilSys.setEquality({1, 2, ["a"] = "b"}, {1, 2}))
		assert(not UtilSys.setEquality({1, 2, ["a"] = "b"}, {1, ["a"] = "b"}))
	end
	function UtilSys.deepcopy(data)
		return json.decode(json.encode(data))
	end
	function UtilSys.arrayConcat(a, b)
		local bCount = #b
		for i = 1, bCount do
			a[#a + 1] = b[i]
		end
	end
	function UtilSys.rectCollides(ax, ay, aw, ah, bx, by, bw, bh)
		return ((ax < (bx + bw)) and ((ax + aw) > bx)
		        and (ay < (by + bh)) and ((ay + ah) > by)
		        and (aw > 0) and (ah > 0) and (bw > 0) and (bh > 0))
	end

-- Client
	local HeadlessClientMetatable = {}
	function HeadlessClientMetatable.__index()
		return UtilSys.noop
	end

	local HeadlessClientSys = setmetatable({}, HeadlessClientMetatable)
	function HeadlessClientSys.isRunning()
		return false
	end

	-- injected by the c client in main.c:jeGame_registerLuaClientBindings()
	local ClientSys = jeClientBindings or HeadlessClientSys  -- luacheck: globals jeClientBindings
	ClientSys.width = 160  -- TODO find a better way to store/accessing this
	ClientSys.height = 120
	function ClientSys.runTests()
		ClientSys.isRunning()
		ClientSys.step()
	end

-- simulation
	local SimulationSys = {}
	SimulationSys.SAVE_VERSION = 1
	SimulationSys.createEvents = {}
	SimulationSys.postCreateEvents = {}
	SimulationSys.destroyEvents = {}
	SimulationSys.stepEvents = {}
	SimulationSys.drawEvents = {}
	SimulationSys.simulation = {}
	function SimulationSys.isRunning()
		return SimulationSys.simulation.created and ClientSys.isRunning()
	end
	function SimulationSys.step()
		local events = SimulationSys.stepEvents
		local eventsCount = #events
		for i = 1, eventsCount do
			events[i]()
		end

		SimulationSys.draw()

		ClientSys.step()
	end
	function SimulationSys.draw()
		local events = SimulationSys.drawEvents
		local eventsCount = #events
		for i = 1, eventsCount do
			events[i]()
		end
	end
	function SimulationSys.destroy()
		UtilSys.log("SimulationSys.destroy()")

		if not SimulationSys.simulation.created then
			return
		end

		for _, event in pairs(SimulationSys.destroyEvents) do
			event()
		end

		for key, _ in pairs(SimulationSys.simulation) do
			SimulationSys.simulation[key] = nil
		end
	end
	function SimulationSys.create()
		if SimulationSys.simulation.created then
			SimulationSys.destroy()
		end

		UtilSys.log("SimulationSys.create()")

		SimulationSys.simulation.created = true
		SimulationSys.simulation.saveVersion = SimulationSys.SAVE_VERSION

		for _, event in pairs(SimulationSys.createEvents) do
			event()
		end

		for _, event in pairs(SimulationSys.postCreateEvents) do
			event()
		end

		return SimulationSys.simulation
	end
	function SimulationSys.dump(filename)
		UtilSys.log("SimulationSys.dump(): filename=%s", filename)

		local saveStr = UtilSys.toComparable(SimulationSys.simulation)

		local file, errMsg = io.open(filename, "w")
		if file == nil then
			UtilSys.err("SimulationSys.dump(): io.open() failed, filename=%s, error=%s", filename, errMsg)
			return
		end

		file:write(saveStr)
		file:close()
	end
	function SimulationSys.save(filename)
		UtilSys.log("SimulationSys.save(): filename=%s", filename)

		local saveStr = json.encode(SimulationSys.simulation)

		local file, errMsg = io.open(filename, "w")
		if file == nil then
			UtilSys.err("SimulationSys.save(): io.open() failed, filename=%s, error=%s", filename, errMsg)
			return
		end

		file:write(saveStr)
		file:close()
	end
	function SimulationSys.load(filename)
		UtilSys.log("SimulationSys.load(): filename=%s", filename)

		local file, errMsg = io.open(filename, "r")
		if file == nil then
			UtilSys.err("SimulationSys.load(): io.open() failed, filename=%s, error=%s", filename, errMsg)
			return
		end

		local gameStr = file:read("*all")
		file:close()

		SimulationSys.simulation = json.decode(gameStr)

		if SimulationSys.simulation.saveVersion > SimulationSys.SAVE_VERSION then
			UtilSys.err("SimulationSys.load(): save version is too new, saveVersion=%d, save.saveVersion=%d",
					   SimulationSys.SAVE_VERSION, SimulationSys.simulation.saveVersion)
			return
		end
		if SimulationSys.simulation.saveVersion < SimulationSys.SAVE_VERSION then
			UtilSys.log("SimulationSys.load(): save version is older, saveVersion=%d, save.saveVersion=%d",
					   SimulationSys.SAVE_VERSION, SimulationSys.simulation.saveVersion)
		end
	end
	function SimulationSys.runTests()
		SimulationSys.destroy()
		SimulationSys.create()

		SimulationSys.step()
		SimulationSys.draw()

		local gameBeforeSave = UtilSys.toComparable(SimulationSys.simulation)
		SimulationSys.dump("test_dump.json")
		SimulationSys.save("test_save.json")
		SimulationSys.load("test_save.json")
		os.remove("test_dump.json")
		os.remove("test_save.json")

		local gameAfterLoad = UtilSys.toComparable(SimulationSys.simulation)
		if gameBeforeSave ~= gameAfterLoad then
			UtilSys.err("SimulationSys.runTests(): Mismatch between SimulationSys.simulation before save and after load: before=%s, after=%s",
						  gameBeforeSave, gameAfterLoad)
		end
	end

-- World
	local WorldSys = {}
	WorldSys.createEvents = {}
	function WorldSys.create()
		UtilSys.log("WorldSys.create()")

		SimulationSys.simulation.world = {}

		for _, event in pairs(WorldSys.createEvents) do
			event()
		end

		return SimulationSys.simulation.world
	end
	function WorldSys.runTests()
		SimulationSys.create()
		assert(SimulationSys.simulation.world ~= nil)
	end
	table.insert(SimulationSys.postCreateEvents, function()
		SimulationSys.simulation.world = WorldSys.create()
	end)

-- Entity
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

		local world = SimulationSys.simulation.world
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

		local worldTagEntities = SimulationSys.simulation.world.tagEntities
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

		local world = SimulationSys.simulation.world
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
		local world = SimulationSys.simulation.world
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
		local world = SimulationSys.simulation.world
		local worldEntities = world.entities

		local entities = {}
		for _, entityId in pairs(SimulationSys.simulation.world.tagEntities[tag] or {}) do
			entities[#entities + 1] = worldEntities[entityId]
		end

		return entities
	end
	function EntitySys.findInBounds(x, y, w, h, tag, getAll)
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

		local world = SimulationSys.simulation.world
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
					local entity = entities[entityId]
					if UtilSys.rectCollides(x, y, w, h, entity.x, entity.y, entity.w, entity.h) then
						if (tag == nil) or (entity.tags[tag] ~= nil) then
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

		return results
	end
	function EntitySys.findAllInBounds(x, y, w, h, tag)
		return EntitySys.findInBounds(x, y, w, h, tag, true)
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

		local destroyedEntities = SimulationSys.simulation.world.destroyedEntities
		destroyedEntities[#destroyedEntities + 1] = entityId
	end
	function EntitySys.create(template)
		local world = SimulationSys.simulation.world
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

		local world = SimulationSys.simulation.world
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
		assert(EntitySys.findInBounds(0, 0, 100, 100, "yolo") == nil)
		assert(#EntitySys.findAllInBounds(0, 0, 100, 100, "yolo") == 0)

		EntitySys.setBounds(entity, 64, 96, 16, 32)
		assert(entity.x == 64 and entity.y == 96 and entity.w == 16 and entity.h == 32)
		assert(UtilSys.setEquality(UtilSys.getKeys(entity.chunks), {"1,1"}))
		assert(world.chunkEntities["1,1"][entity.chunks["1,1"]] == entity.id)
		assert(#EntitySys.findAllInBounds(0, 0, 100, 100, "yolo") == 1)
		assert(EntitySys.findInBounds(0, 0, 100, 100, "yolo"))
		assert(EntitySys.findInBounds(0, 0, 64, 96, "yolo") == nil)
		assert(EntitySys.findInBounds(0, 0, 65, 97, "yolo"))
		assert(EntitySys.findInBounds(80, 128, 1, 1, "yolo") == nil)
		assert(EntitySys.findInBounds(79, 127, 1, 1, "yolo"))

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
		assert(#EntitySys.findAllInBounds(0, 0, 100, 100, "ysg") == 0)
		for i = 1, numEntities do
			entity = EntitySys.create()
			EntitySys.setBounds(entity, i, i, 1, 1)
			EntitySys.tag(entity, tostring(i))
			EntitySys.tag(entity, "ysg")

			assert(EntitySys.find(tostring(i)) == entity)

			entities[#entities + 1] = entity
		end

		assert(UtilSys.setEquality(EntitySys.findAll("ysg"), entities))
		assert(#EntitySys.findAllInBounds(0, 0, 100, 100, "ysg") == numEntities)

		for _, entityToDestroy in ipairs(entities) do
			EntitySys.destroy(entityToDestroy)
		end

		assert(UtilSys.setEquality(EntitySys.findAll("ysg"), {}))

		EntitySys.ENTITY_CHUNK_SIZE = entityChunkSizeBackup
	end
	table.insert(WorldSys.createEvents, function()
		local world = SimulationSys.simulation.world
		world.entities = {}
		world.tagEntities = {}
		world.chunkEntities = {}
		world.destroyedEntities = {}
	end)

-- Template
	local templateSys = {}
	function templateSys.add(templateId, template)
		local templates = SimulationSys.simulation.templates
		local currentTemplate = templates[templateId]
		if currentTemplate == nil then
			templates[templateId] = template
		end
		return template
	end
	function templateSys.runTests()
		SimulationSys.create()
		assert(SimulationSys.simulation.templates ~= nil)

		local template = {["x"] = 2, ["tags"] = {["yee"] = true}}
		templateSys.add("yee", template)
		EntitySys.create(SimulationSys.simulation.templates["yee"])
		assert(EntitySys.find("yee").x == 2)
		assert(EntitySys.find("yee").tags["yee"] ~= nil)
	end
	table.insert(SimulationSys.createEvents, function()
		SimulationSys.simulation.templates = {}
	end)

-- Screen
	local screenSys = {}
	screenSys.drawEvents = {}
	screenSys.ENTITY_TEMPLATE = {
		["w"] = ClientSys.width,
		["h"] = ClientSys.height,
		["tags"] = {
			["screen"] = true,
		}
	}
	function screenSys.get()
		local screen = EntitySys.find("screen")
		if screen == nil then
			screen = EntitySys.create(screenSys.ENTITY_TEMPLATE)
		end

		return screen
	end
	table.insert(WorldSys.createEvents, function()
		screenSys.get()
	end)
	table.insert(SimulationSys.drawEvents, function()
		local screen = screenSys.get()

		local events = screenSys.drawEvents
		local eventsCount = #events
		for i = 1, eventsCount do
			events[i](screen)
		end
	end)

-- Sprite
	local spriteSys = {}
	spriteSys.GRID_SIZE = 8
	function spriteSys.add(spriteId, u, v, w, h)
		local gridSize = spriteSys.GRID_SIZE

		local sprites = SimulationSys.simulation.sprites
		local sprite = sprites[spriteId]
		if sprite == nil then
			sprite = {
				["spriteId"] = spriteId,
				["u1"] = u,
				["v1"] = v,
				["u2"] = u + (w or gridSize),
				["v2"] = v + (h or gridSize),
			}
			sprites[spriteId] = sprite
		end

		return sprite
	end
	function spriteSys.attach(entity, sprite)
		local spriteId = sprite.spriteId

		entity.spriteId = spriteId
		entity.u1 = sprite.u1
		entity.v1 = sprite.v1
		entity.u2 = sprite.u2
		entity.v2 = sprite.v2
		EntitySys.tag(entity, "sprite")
	end
	function spriteSys.detach(entity)
		entity.spriteId = nil
		entity.u = nil
		entity.v = nil
	end
	function spriteSys.runTests()
		SimulationSys.create()

		local entity = EntitySys.create()
		local testSprite = spriteSys.add("test", 40, 0, 8, 8)

		spriteSys.attach(entity, testSprite)
		assert(entity.u1 == 40)
		assert(entity.u2 == 48)
		assert(entity.spriteId == "test")

		spriteSys.detach(entity, testSprite)
		assert(entity.spriteId == nil)
	end
	table.insert(SimulationSys.createEvents, function()
		SimulationSys.simulation.sprites = {}
		spriteSys.add("invalid", 0, 0)
	end)
	table.insert(EntitySys.tagEvents, function(entity, tag, tagId)
		if tag == "sprite" and tagId ~= nil then
			local sprite = SimulationSys.simulation.sprites[entity.spriteId]
			entity.u1 = sprite.u1
			entity.v1 = sprite.v1
			entity.u2 = sprite.u2
			entity.v2 = sprite.v2
			entity.z = entity.z or 0
			entity.r = entity.r or 255
			entity.g = entity.g or 255
			entity.b = entity.b or 255
			entity.a = entity.a or 255
		end
	end)
	table.insert(screenSys.drawEvents, function(screen)
		local world = SimulationSys.simulation.world
		local entities = world.entities
		local spriteEntityIds = world.tagEntities["sprite"] or {}
		local spriteEntityIdsCount = #spriteEntityIds

		for i = 1, spriteEntityIdsCount do
			ClientSys.drawSprite(entities[spriteEntityIds[i]], screen)
		end
	end)

-- Engine
	local EngineSys = {}
	EngineSys.systems = {
		["UtilSys"] = UtilSys,
		["ClientSys"] = ClientSys,
		["SimulationSys"] = SimulationSys,
		["WorldSys"] = WorldSys,
		["EntitySys"] = EntitySys,
		["templateSys"] = templateSys,
		["screenSys"] = screenSys,
		["spriteSys"] = spriteSys,
	}
	function EngineSys.runTests()
		UtilSys.log("EngineSys.runTests(): Running automated tests")

		for systemName, system in pairs(EngineSys.systems) do
			if system.runTests then
				UtilSys.log("EngineSys.runTests(): Running tests for %s", systemName)
				system.runTests()
			end
		end
	end


-- ----------------- Game -----------------
-- Wall
	local wallSys = {}
	table.insert(SimulationSys.createEvents, function()
		wallSys.template = templateSys.add("wall", {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "wallBlack",
			["tags"] = {
				["sprite"] = true,
				["solid"] = true,
			}
		})
		spriteSys.add("wallBlack", 0, 40)
	end)

-- Player
	local playerSys = {}
	table.insert(SimulationSys.createEvents, function()
		playerSys.template = templateSys.add("player", {
			["w"] = 8,
			["h"] = 8,
			["spriteId"] = "playerLeft",
			-- ["animationId"] = "playerLeft",
			-- ["animationPos"] = 0,
			-- ["animationSpeed"] = 0.1,
			-- ["speedX"] = 0,
			-- ["speedY"] = 0,
			-- ["forceX"] = 0,
			-- ["forceY"] = 0,
			["tags"] = {
				["sprite"] = true,
				-- ["animation"] = true,
				["solid"] = true,
				-- ["physics"] = true,
				["player"] = true,
			}
		})
		spriteSys.add("playerLeft", 8, 0, 8, 8)
	end)

-- Game
	local GameSys = {}
	GameSys.SAVE_FILE = ".\\game_save.json"
	function GameSys.populateTestWorld()
		for i = 1, 3 do
			local wall = EntitySys.create(wallSys.template)
			EntitySys.setBounds(wall, 24 + ((i - 1) * 32), 24 + ((i - 1) * 32), 48, 8)
		end

		local wallLeft = EntitySys.create(wallSys.template)
		EntitySys.setBounds(wallLeft, 0, 0, 8, ClientSys.height)

		local wallRight = EntitySys.create(wallSys.template)
		EntitySys.setBounds(wallRight, ClientSys.width - 8, 0, 8, ClientSys.height)

		local player = EntitySys.create(playerSys.template)
		EntitySys.setPos(player, 32, 8)
	end
	function GameSys.run()
		SimulationSys.create()
		GameSys.populateTestWorld()

		while SimulationSys.isRunning() do
			SimulationSys.step()
		end

		SimulationSys.save(GameSys.SAVE_FILE)
		SimulationSys.destroy()
	end


-- ----------------- Main -----------------
-- Main
	local function main()
		EngineSys.runTests()
		GameSys.run()
	end

	main()
