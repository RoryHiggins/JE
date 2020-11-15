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


## Simulation:addSystem(system)
Adds the specified system to the simulation, if not already present.  Returns an instance of that system.

Systems are uniquely keyed by system.SYSTEM_NAME. If a system with that key does not exist, this method will instantiate the system as follows:

- Creates a new table, systemInstance
- set system as the metatable for systemInstance
- registers systemInstance into the simulation's SYSTEM_NAME to system instance mapping

**Arguments**

- "system" - metatable used for the system instance

**Returns**

- instance of the specified system

**Examples**

```lua
local Entity = require("engine/systems/entity")

local PhysicsSys = {}

-- unique string key for the system,
PhysicsSys.SYSTEM_NAME = "physicsSys"

-- triggered once when the simulation
-- triggered immediately if the system is added after simulation is created
function PhysicsSys:onInit(simulation)
	self.simulation = simulation
	self.entitySys = self.simulation:addSystem(Entity)
end

-- triggered once per simulation step (60 times a second, when run from the client)
function PhysicsSys:onStep()
	for _, entity in ipairs(self.entitySys:findAll("physics")) do
		self.entitySys:movePos(entity, 0, 1)  -- add 1 to y every frame; not the most exciting gravity physics
	end
end
```


## Simulation:run()
Runs the simulation loop until completion.

When run from the client, this ends when the client window is closed or ESC is pressed.

When run headless (from a lua interpreter) the simulation will start, step once, and then stop.

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
Saves simulation state as gzipped JSON to the given filename.  Only state in simulation.state is saved.

There are some major restrictions simulation.state must adhere to, in order to support saving:

- no table recursion, i.e. a table cannot contain itself, directly or indirectly
- primitive types only, i.e. must only be composed of tables, strings, numbers, bool, and nil
- no sparse arrays (nils in tables containing only number keys)
- cannot mix string keys and number keys in tables

There are also some restrictions which simulation.state should adhere to, else information will be lost:

- no metatables - tables will not include their metatable nor metatable members when the save is loaded
- only store one reference to a table - each reference will be its own table when the save is loaded

**Arguments**

- "filename" - string filename where save should be written to

**Returns**

- true if successful, else false

**Examples**

```lua
simulation:save("save.dat")
```


## Simulation:load(filename)
Loads simulation state as gzipped JSON from the given filename.  If successful, replaces what is already in simulation.state.

**Arguments**

- "filename" - string filename where save should be read from

**Returns**

- true if successful, else false

**Examples**

```lua
simulation:load("save.dat")
```
