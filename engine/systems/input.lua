local client = require("engine/systems/client")

local Input = {}
Input.SYSTEM_NAME = "input"
Input.CLIENT_INPUT_MAP = {
	["left"] = "inputLeft",
	["right"] = "inputRight",
	["up"] = "inputUp",
	["down"] = "inputDown",
	["a"] = "inputA",
	["b"] = "inputB",
	["x"] = "inputX",
	["y"] = "inputY",
}
function Input:stepInputs()
	local previousInputs = self.inputs or {}

	local inputs = {}

	for inputKey, clientInputKey in pairs(self.CLIENT_INPUT_MAP) do
		local input = {}
		input.down = client.state[clientInputKey]

		input.pressed = false
		input.released = false
		input.framesDown = 0
		if previousInputs[inputKey] then
			input.pressed = input.down and not previousInputs[inputKey].down
			input.released = not input.down and previousInputs[inputKey].down
			if input.down then
				input.framesDown = previousInputs[inputKey].framesDown + 1
			end
		end

		inputs[inputKey] = input
	end

	self.inputs = inputs
end
function Input:get(inputKey)
	return self.inputs[inputKey].down
end
function Input:getPressed(inputKey)
	return self.inputs[inputKey].pressed
end
function Input:getReleased(inputKey)
	return self.inputs[inputKey].released
end
function Input:onInit(simulation)
	self.simulation = simulation
	self:stepInputs()
end
function Input:onStep()
	self:stepInputs()
end

return Input
