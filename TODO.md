
Reindexing
----------
entity.indexed table, for indexed values of data
entitySys:onEntityReindex
	entitySys:reindexBounds
	entitySys:reindexTags

Client indexing - IN ADDITION TO simulation indexing (does not replace it)
- Static entity sprite indexing
- 


Static initialization phase
---------------------------

Init = static initialization only?

game.static created and populated all in one game event
world.static created and populated all in one world event


Render targets
--------------

Add texture atlas support:

- (re-)allocate and reset an area with a unique key name
- render texture support, with the caveat that switching render target flushes draw calls
- make textureId a required field in draw calls, and dynamically compute offsets
- allow loading unknown texture sizes from a file

```lua
client.defineTexture({["textureId"] = ..., ["w"] = ..., ["h"] = ...})
client.defineTexture({["textureId"] = ..., ["filename"] = ...})
```

Add viewports support, particularly for the GUI.  Planned interface (by example):
```lua
-- clear 
client.defineTexture({["textureId"] = "camera_1", ["w"] = 160, ["h"] = 120})
client.drawBegin({["textureId"] = "camera_1", ["r"] = 1, ["g"] = 1, ["b"] = 1, ["a"] = 1})
-- draw the game with all its viewports onto a render target
self:broadcast("onDraw")
client.drawEnd()

-- then draw the game on the screen (may be a different resolution than the game)

-- NEED TO ADD TEXTURE ATLAS SUPPORT FIRST FOR FBO TO BE DRAWABLE!
-- might also want to be smarter on the simulation side (integer scaling, keep aspect ratio)
client.targetScreen()
client.drawBegin()
client.drawSprite({
	["textureId"] = "game",
	["x1"] = 0,
	["y1"] = 0,
	["x2"] = client.state.w,
	["y2"] = client.state.h,
	["u1"] = 0,
	["v1"] = 0,
	["u2"] = 160,
	["v2"] = 120,
})
self:broadcast("onDrawScreen")
client.drawEnd()
```

Configuration
-------------
By default one file for all, but support multiple files with a defined parse order
Lives in the game/ folder

Flow for configuring the client:
	client.configure(static.client)
	client.open()

Rename minimal -> minimal_example
Add minimal_client_example showing the minimum needed to have a


Editor thoughts
---------------

GUI at native scale

Keyboard + mouse focused

- Mouse wheel to zoom
- Space + left mouse to drag
- ESC to close focused window or to unfocus text input
- HOLD ESC for a few seconds for a prompt to close the 

GUI elements

- Buttons
- Checkboxes
- Text input
- Object model text input + validation + tree view (components, resources, arbitrary JSON-like data, etc)

GUI windows
- Prompt windows (yes/no). enter=yes and ESC=no

Command line as a first-class citizen

- / button to start a command.  / key to start typing a command
- Display list of recent and important commands
- Fuzzy autocomplete with tab
- Zero-argument commands (deselect all, )
- Command mode: on keyboard, press / to start a command

	Ordered by most recently used
	Optional 

- Filter: filtered out entities are translucent and cannot be selected
- Selection:

	hold left mouse button and drag to select area
	double-tap action to select all unfiltered entities
	tap shift to add to selection
	hold shift and drag to extend selection
	tap ESC to deselect

- Selection mode: add rectangle, remove rectangle
- 
- Modifiers: 

Editor support

- Engine/game control of screen size and scale
- Exposing cursor position + button state
- Exposing raw keyboard input (only for UIs)
- Text rendering (tested with UTF-8)
- Template instantiator
- UI Element: Dialog
- UI Element: Button
- UI Element: Textbox



Text rendering
--------------
Bounding
- Overflow text onto next row if width is reached.
- Clamp text within width/height bounds.

Rich text support: font loading, unicode, and kerning.  Bring alcohol.


Misc Issues
-----------

- Releasing with a specified game does not change the client's default target
