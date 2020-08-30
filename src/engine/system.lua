local System = {}
function System:addDependency(systemClass)
	assert(systemClass ~= nil)

	self[systemClass.SYSTEM_NAME.."Sys"] = self.simulation:addSystem(systemClass)
end
function System:addDependencies(...)
	local dependencies = {...}
	assert(#dependencies > 0)

	for _, dependency in ipairs(dependencies) do
		self:addDependency(dependency)
	end
end
function System.new(systemName)
	local systemClass = {}
	systemClass.__index = systemClass
	systemClass.SYSTEM_NAME = systemName

	System.__index = System
	setmetatable(systemClass, System)

	return systemClass
end

return System