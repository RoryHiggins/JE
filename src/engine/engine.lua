local util = require("src/engine/util")

local EngineSys = {}
EngineSys.components = {
	["UtilSys"] = require("src/engine/util"),
	["ClientSys"] = require("src/engine/client"),
	["SimulationSys"] = require("src/engine/simulation"),
	["WorldSys"] = require("src/engine/world"),
	["EntitySys"] = require("src/engine/entity"),
	["TemplateSys"] = require("src/engine/template"),
	["ScreenSys"] = require("src/engine/screen"),
	["SpriteSys"] = require("src/engine/sprite"),
	-- ["TextSys"] = require("src/engine/text"),
}
function EngineSys.runTests()
	util.log("EngineSys.runTests(): Running automated tests")

	for componentName, component in pairs(EngineSys.components) do
		if component.runTests then
			util.log("EngineSys.runTests(): Running tests for %s", componentName)
			component.runTests()
		else
			util.log("EngineSys.runTests(): No tests for %s", componentName)
		end
	end
end

return EngineSys
