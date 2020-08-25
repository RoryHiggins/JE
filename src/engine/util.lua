local json = require("lib/json")

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

function UtilSys.sign(x)
	if x > 0 then
		return 1
	elseif x < 0 then
		return -1
	end

	return 0
end
function UtilSys.boolToNumber(boolVal)
	-- it's truly astonishing there is nothing builtin for this...
	if boolVal then
		return 1
	else
		return 0
	end
end

function UtilSys.deepcopy(data)
	return json.decode(json.encode(data))
end
function UtilSys.tableExtend(dest, ...)
	local arg = {...}
	for _, overrides in ipairs(arg) do
		overrides = UtilSys.deepcopy(overrides)
		for key, val in pairs(overrides) do
			dest[key] = val
		end
	end

	return dest
end
function UtilSys.arrayConcat(a, b)
	local bCount = #b
	for i = 1, bCount do
		a[#a + 1] = b[i]
	end
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
function UtilSys.rectCollides(ax, ay, aw, ah, bx, by, bw, bh)
	return ((ax < (bx + bw)) and ((ax + aw) > bx)
	        and (ay < (by + bh)) and ((ay + ah) > by)
	        and (aw > 0) and (ah > 0) and (bw > 0) and (bh > 0))
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

return UtilSys
