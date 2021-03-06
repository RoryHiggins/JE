local client = require("engine/client/client")

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
	["mouseLeft"] = "inputMouseLeft",
	["mouseMiddle"] = "inputMouseMiddle",
	["mouseRight"] = "inputMouseRight",
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
	self.mouseX = client.state["inputMouseX"]
	self.mouseY = client.state["inputMouseY"]
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
function Input:getMousePos()
	return self.mouseX, self.mouseY
end
function Input:onInit(simulation)
	self.simulation = simulation
	self:stepInputs()
end
function Input:onStep()
	self:stepInputs()
end

return Input
