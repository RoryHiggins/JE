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


## Simulation:run()
Runs the simulation loop until completion.  Triggers the following events:

- onInitialize for each 

When run from the client, this ends when the client window is closed or ESC is pressed.

When run headless (from a lua interpreter) the simulation will initialize, start, step once, and then stop.

**Arguments**

**Returns**

**Examples**

```lua
local simulation = Simulation.new()
simulation:addSystem(MyGame)
simulation:run()
```


## Simulation:stop()
Stop the simulation if it is currently running.  Can be called mid-simulation to exit early.  Triggers onStop event.  Always called at the end of simulation.run()



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

In order to support saving, simulation.state must be [Serializable](../abstractions/serializable.html).

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


## Simulation:constants
Constants defining the behavior of the simulation.  Populated during the onInit event.  Note when modifying constants:

- Should only be modified during onInit.
- Should be deterministically initialized: each init() event should produce an identical constants object
- Should be [Serializable](../abstractions/serializable.html).

## Simulation:input
Input from the client for the current simulation step (screen, fps, keyboard/controller input status, random seed, etc).
Non deterministic.  Should not be modified by game code.

## Simulation:state
Current simulation state (including the world).  Note when modifying state:

 - Should be Serializable [Serializable](../abstractions/serializable.html).  Must be serializable if save()/load() is used
 - Should be deterministically updated when provided the same simulation.constants and the same simulation.input for each step event
