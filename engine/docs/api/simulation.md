# API Reference - Simulation

[TOC]

## Simulation.new()
Create a new simulation.

**Arguments**

**Returns**

- simulation object which can be run with simulation:run()

**Examples**

```lua
local simulation = Simulation.new()
simulation:addSystem(MyGame)
simulation:run()
```


## Simulation:addSystem(prototype)
Instantiates and adds system to the simulation, if not already added.

**Arguments**

- "prototype" - table used as the prototype for the system instance

**Returns**

- instance of the requested system

**Examples**

```lua
local Entity = require("engine/entity")

local PhysicsSys = {}

-- unique string key for the system,
PhysicsSys.SYSTEM_NAME = "physicsSys"

-- triggered once when the simulation
-- triggered immediately if the system is added after simulation is created
PhysicsSys.onSimulationCreate(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
end

-- triggered once per simulation step (60 times a second, when run from the client)
PhysicsSys.onSimulationStep()
	for _, entity in ipairs(self.entitySys:findAll("physics")) do
		self.entitySys:movePos(entity, 0, 1)  -- not the most interesting gravity physics
	end
end
```


## Simulation:run()
Runs the simulation loop until completion.

When run from the client, this ends when the client window is closed or ESC is pressed.

When run as a script (headless), the simulation will create and be destroyed.

**Arguments**

**Returns**

**Examples**

```lua
local simulation = Simulation.new()
simulation:addSystem(MyGame)
simulation:run()
```


## Simulation:broadcast(event, ...)
Trigger an event.  For each system, if there is an method matching the *event* name to handle the event,
call this event handler with the variadic arguments (...).

**Arguments**

- "event" - string event name key

- "..." - variadic arguments to forward to the event handler

**Returns**

**Examples**

```lua
MessageSystem = {}
MessageSystem.SYSTEM_NAME = "MessageSystem"
function MessageSystem:onMessage(message)
	print(message)
end

simulation:addSys(SomeSystem)
simulation:broadcast("onMessage", "hello!")
-- prints "hello!"
```


## Simulation:dump(filename)
Serializes as much of the simulation as possible to JSON,
and saves this as JSON to the given filename.

Note: this is a very slow operation, and the output can be quite large.

**Arguments**

- "filename" - string filename where the simulation dump json should be written to

**Returns**

- true if successful, else false

**Examples**

```lua
simulation:dump("simulation_dump.json")
```


## Simulation:save(filename)
Saves simulation state to the given filename.  Only state in simulation.state is saved.
There are some major restrictions simulation.state must adhere to, in order to support saving:

- primitive types only, i.e. must only be composed of tables, strings, numbers, bool, and nil
- no table recursion, i.e. a table cannot contain itself, directly or indirectly
- no sparse arrays (nils in tables containing only number keys)
- cannot mix string keys and number keys in tables
- no metatables

**Arguments**

- "filename" - string filename where save should be written to

**Returns**

- true if successful, else false

**Examples**

```lua
simulation:save("save.dat")
```


## Simulation:load(filename)
Loads simulation state from the given filename.  If successful, replaces what is already in simulation.state.

**Arguments**

- "filename" - string filename where save should be read from

**Returns**

- true if successful, else false

**Examples**

```lua
simulation:load("save.dat")
```
