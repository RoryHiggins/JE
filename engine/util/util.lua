local json = require("engine/lib/json/json")

local util = {}
util.SYSTEM_NAME = "util"

function util.noop()
end

function util.sign(x)
	if x > 0 then
		return 1
	elseif x < 0 then
		return -1
	end

	return 0
end
function util.boolToNumber(boolVal)
	-- it's truly astonishing there is nothing builtin for this...
	if boolVal then
		return 1
	else
		return 0
	end
end

function util.deepcopy(data)
	return json.decode(json.encode(data))
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
function util.arrayConcat(a, b)
	local bCount = #b
	for i = 1, bCount do
		a[#a + 1] = b[i]
	end
end
function util.getKeys(input)
	local keys = {}

	for key, _ in pairs(input) do
		keys[#keys + 1] = key
	end
	table.sort(keys)

	return keys
end
function util.getEscapedString(input)
	return (
		input
		:gsub('\\', '\\\\')
		:gsub('\n', '\\n')
		:gsub('\r', '\\r')
		:gsub('\"', '\\"')
		:gsub("[^%w%p%s]", "?")
	)
end
function util.toComparable(input, stack, indentation)
	local inputType = type(input)

	indentation = indentation or ""

	if inputType == "number" then
		return tostring(input)
	elseif inputType == "string" then
		return "\""..util.getEscapedString(input).."\""
	elseif inputType == "table" then
		stack = stack or {}
		for _, seenInput in pairs(stack) do
			if tostring(input) == tostring(seenInput) then
				return "\"<unserializable table recursion>\""
			end
		end

		stack[#stack + 1] = input

		-- get keys in sorted order (to be deterministic)
		local keys = util.getKeys(input)

		local fieldStrings = {}
		local innerIndentation = indentation.."\t"
		for _, key in ipairs(keys) do
			local val = input[key]

			local fieldString = string.format(
				"\n%s\"%s\": %s",
				innerIndentation,
				tostring(key),
				util.toComparable(val, stack, innerIndentation)
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
		return util.toComparable(tostring(input))
	elseif inputType == "userdata" then
		return "\"<unserializable userdata>\""
	else
		return tostring(input)
	end
end
function util.getValues(input)
	local values = {}

	for _, value in pairs(input) do
		values[#values + 1] = value
	end

	return values
end
function util.sorted(values)
	values = util.deepcopy(values)
	table.sort(values, function (a, b) return util.toComparable(a) < util.toComparable(b) end)

	return values
end
function util.createSet(values)
	values = util.sorted(values)

	local duplicateIndices = {}

	for i = 1, #values do
		local value = values[i]
		for j = 1, (i - 1) do
			if util.toComparable(value) == util.toComparable(values[j]) then
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
function util.setEquality(a, b)
	local setA = util.createSet(util.getValues(a))
	local setB = util.createSet(util.getValues(b))

	return util.toComparable(setA) == util.toComparable(setB)
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
	assert(util.getKeys({["a"] = 1})[1] == "a")
	assert(util.getEscapedString("\n\"\0") == "\\n\\\"?")
	assert(util.toComparable(nil) == util.toComparable(nil))
	assert(util.toComparable(nil) ~= util.toComparable(1))
	assert(util.toComparable(1) == util.toComparable(1))
	assert(util.toComparable(1) ~= util.toComparable(2))
	assert(util.toComparable("a") == util.toComparable("a"))
	assert(util.toComparable("a") ~= util.toComparable("b"))
	assert(util.toComparable({["a"] = 1, ["b"] = 2}) == util.toComparable({["a"] = 1, ["b"] = 2}))
	assert(util.toComparable({["a"] = 1, ["b"] = 2}) ~= util.toComparable({["a"] = 1, ["b"] = 2, ["c"] = 3}))
	assert(util.toComparable({["a"] = 1, ["b"] = 2}) ~= util.toComparable({["a"] = 2, ["b"] = 2}))
	assert(util.toComparable(_G) == util.toComparable(_G))
	assert(util.toComparable(util.getValues({1, 2, 3})) == util.toComparable({1, 2, 3}))
	assert(util.toComparable(util.sorted({3, 2, 1})) == util.toComparable({1, 2, 3}))
	assert(util.toComparable(util.sorted(util.getValues({["a"] = 1, ["b"] = 2, ["c"] = 3}))) == util.toComparable({1, 2, 3}))
	assert(util.toComparable(util.createSet({1, 2, 3, 3})) == util.toComparable({1, 2, 3}))
	assert(util.setEquality({}, {}))
	assert(util.setEquality({1, 2}, {2, 1, 1}))
	assert(util.setEquality({1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}, {1, 2, ["a"] = "b", ["c"] = {["d"] = 1}}))
	assert(not util.setEquality({["c"] = {["d"] = 1}}, {["c"] = {["d"] = 2}}))
	assert(not util.setEquality({1, 2, ["a"] = "b"}, {1, 2}))
	assert(not util.setEquality({1, 2, ["a"] = "b"}, {1, ["a"] = "b"}))
end

util.json = json

return util
