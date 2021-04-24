local log = require("engine/util/log")
local json = require("engine/lib/json/json")

local util = {}
util.SYSTEM_NAME = "util"

function util.noop()
end
function util.deepcopy(data)
	return json.decode(json.encode(data))
end

function util.sign(x)
	if x > 0 then
		return 1
	elseif x < 0 then
		return -1
	end

	return 0
end
function util.moduloAddSkipZero(x, y, base)
	local result = (x + y) % base
	if (result == 0) then
		if util.sign(y) < 0 then
			result = (result - 1) % base
		else
			result = (result + 1) % base
		end
	end
	return result
end
function util.boolGetValue(boolVal)
	-- it's truly astonishing there is nothing builtin for this...
	if boolVal then
		return 1
	else
		return 0
	end
end

function util.stringEscaped(input)
	return (
		input
		:gsub('\\', '\\\\')
		:gsub('\n', '\\n')
		:gsub('\r', '\\r')
		:gsub('\"', '\\"')
		:gsub("[^%w%p%s]", "?")
	)
end
function util.getComparable(input, stack, indentation)
	local inputType = type(input)

	indentation = indentation or ""

	if inputType == "number" then
		return tostring(input)
	elseif inputType == "string" then
		return "\""..util.stringEscaped(input).."\""
	elseif inputType == "table" then
		stack = stack or {}
		for _, seenInput in pairs(stack) do
			if tostring(input) == tostring(seenInput) then
				return "\"<unserializable table recursion>\""
			end
		end

		stack[#stack + 1] = input

		-- get keys in sorted order (to be deterministic)
		local keys = util.tableGetKeys(input)

		local fieldStrings = {}
		local innerIndentation = indentation.."\t"
		for _, key in ipairs(keys) do
			local val = input[key]

			local fieldString = string.format(
				"\n%s\"%s\": %s",
				innerIndentation,
				tostring(key),
				util.getComparable(val, stack, innerIndentation)
			)
			fieldStrings[#fieldStrings + 1] = fieldString
		end

		local result = indentation.."{"..table.concat(fieldStrings, ", ").."\n"..indentation.."}"
		if indentation ~= "" then
			result = "\n"..result
		end

		return result
	elseif inputType == "nil" then
		return "null"
	elseif inputType == "function" then
		return util.getComparable(tostring(input))
	elseif inputType == "userdata" then
		return "\"<unserializable userdata>\""
	else
		return tostring(input)
	end
end

function util.tableGetKeys(input)
	local keys = {}

	for key, _ in pairs(input) do
		keys[#keys + 1] = key
	end

	return keys
end
function util.tableGetValues(input)
	local values = {}

	for _, value in pairs(input) do
		values[#values + 1] = value
	end

	return values
end
function util.tableHasValue(input, target_value)
	for _, value in pairs(input) do
		if value == target_value then
			return true
		end
	end

	return false
end
function util.tableConcat(a, b)
	local bCount = #b
	for i = 1, bCount do
		a[#a + 1] = b[i]
	end
	return a
end
function util.tableDeepSort(values)
	table.sort(values, function (a, b) return util.getComparable(a) < util.getComparable(b) end)
	return values
end
function util.tableGetSorted(values)
	return util.tableDeepSort(util.deepcopy(values))
end
function util.tableExtend(dest, overrides)
	for key, value in pairs(overrides) do
		if type(value) == "table" then
			local destTable = dest[key]
			if type(dest[key]) ~= "table" then
				destTable = {}
				dest[key] = destTable
			end
			util.tableExtend(destTable, value)
		else
			dest[key] = value
		end
		dest[key] = value
	end

	return dest
end

function util.setCreate(values)
	values = util.tableGetSorted(values)

	local duplicateIndices = {}

	for i = 1, #values do
		local value = values[i]
		for j = 1, (i - 1) do
			if util.getComparable(value) == util.getComparable(values[j]) then
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
function util.setEquals(a, b)
	local setA = util.setCreate(util.tableGetValues(a))
	local setB = util.setCreate(util.tableGetValues(b))

	return util.getComparable(setA) == util.getComparable(setB)
end

function util.rectCollides(ax, ay, aw, ah, bx, by, bw, bh)
	return ((ax < (bx + bw)) and ((ax + aw) > bx)
	        and (ay < (by + bh)) and ((ay + ah) > by)
	        and (aw > 0) and (ah > 0) and (bw > 0) and (bh > 0))
end

function util.writeDataUncompressed(filename, dataStr)
	local file, errMsg = io.open(filename, "w")
	if file == nil then
		log.error("io.open() failed, filename=%s, error=%s", filename, errMsg)
		return false
	end

	file:write(dataStr)
	file:close()

	return true
end
function util.readDataUncompressed(filename)
	local file, errMsg = io.open(filename, "r")
	if file == nil then
		log.error("io.open() failed, filename=%s, error=%s", filename, errMsg)
		return
	end

	local dataStr = file:read("*all")
	file:close()

	return dataStr
end

function util.onRunTests()
	util.noop()
	log.assert(util.tableGetKeys({["a"] = 1})[1] == "a")
	log.assert(util.stringEscaped("\n\"\0") == "\\n\\\"?")
	log.assert(util.getComparable(nil) == util.getComparable(nil))
	log.assert(util.getComparable(nil) ~= util.getComparable(1))
	log.assert(util.getComparable(1) == util.getComparable(1))
	log.assert(util.getComparable(1) ~= util.getComparable(2))
	log.assert(util.getComparable("a") == util.getComparable("a"))
	log.assert(util.getComparable("a") ~= util.getComparable("b"))
	log.assert(util.getComparable({["a"] = 1, ["b"] = 2}) == util.getComparable({["a"] = 1, ["b"] = 2}))
	log.assert(util.getComparable({["a"] = 1, ["b"] = 2}) ~= util.getComparable({["a"] = 1, ["b"] = 2, ["c"] = 3}))
	log.assert(util.getComparable({["a"] = 1, ["b"] = 2}) ~= util.getComparable({["a"] = 2, ["b"] = 2}))
	log.assert(util.getComparable(_G) == util.getComparable(_G))
	log.assert(util.getComparable(util.tableGetValues({1, 2, 3})) == util.getComparable({1, 2, 3}))
	log.assert(util.getComparable(util.tableDeepSort({3, 2, 1})) == util.getComparable({1, 2, 3}))
	log.assert(util.getComparable(util.tableDeepSort(util.tableGetValues({["a"] = 1, ["b"] = 2}))) == util.getComparable({1, 2}))
	log.assert(util.getComparable(util.setCreate({1, 2, 3, 3})) == util.getComparable({1, 2, 3}))
	log.assert(util.setEquals({}, {}))
	log.assert(util.setEquals({1, 2}, {2, 1, 1}))
	log.assert(util.setEquals({1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}, {1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}))
	log.assert(not util.setEquals({["c"] = {["d"] = 1}}, {["c"] = {["d"] = 2}}))
	log.assert(not util.setEquals({1, 2, ["a"] = "b"}, {1, 2}))
	log.assert(not util.setEquals({1, 2, ["a"] = "b"}, {1, ["a"] = "b"}))
end

util.json = json

return util
