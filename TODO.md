
# LD48 -> Master merge

- Remove references to ld48 systems from the engine

Nice to haves:
- Move background.lua system to engine
- Move editor.lua system to engine (might spawn more work)

# Sweep to ensure all state is appropriately put in state or constants

# Add "JE_CHECK()" for release non-fatal asserts
- Sweep to replace verbose if(blah) {JE_ERR(...); ok = false; }

# Deglobalize client table
- Pass client table in as an argument rather than as a global
- Make client a real system
- HeadlessClient -> BaseClient, and have the provided client _override_ the BaseClient
- Access client instance through simulation (getSystem())


# Asset management at the engine level
- Level, audio, sprite, spritesheets


# World loading logic should be an engine system
- Lots of decoupling cleanup to do
- Command-line argument to force starting in a specific level

# Proper rectangle rendering

# Draw sprite without attaching one

# Raw keyboard input, including pressed/released detection

# Event handlers for keyboard input

# Rethink keyboard controls for editor

# Multiline text

- Fix text size estimation for multiline text

# Remove need for manual uv management

# Simplify bulk template instance creation
- Template inheritance?

# Move physics and physics material to engine
- Death should not be a physics material

Capture stack trace on a lua error
Dump simulation state + stack trace on a lua error
- Dump the first error only

# Audio

wav loading and playing

proper audio mixing

volume control

ogg streaming is a nice-to-have

cleanup:
- test coverage
- exclude libogg in headless client builds?


# jeString guarantee null termination
Capacity always at least 1 > than count.  added capacity is null padded


# Image.c tests


# Move as much logic as possible out of simulation.lua

world -> a new systems/world.lua
input -> systems/input.lua


# Refactor input to support multiple source ids


# Render targets

Client: add support for triggering a viewport draw, with the given viewport

Engine: add support for multiple game cameras
- Expose the current camera in the render event
- replace camera.cameraTarget with a CameraFollow component
- input source id == camera id make sense?
```
for _, camera in pairs(self.entitySys:findAll("camera"))
	client.drawBegin()  -- makes drawReset useless
	self:broadcast("onDraw", true, camera)
	client.drawEnd(camera)
end
```


# Animations


# Configuration


# Client-side render state indexing


# Client-side tag+bounds indexing


# Editor

gui draw loop (begin, end with gui camera)
- GUI at native screen resolution, on top of game viewports

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


# Improved text rendering
Bounding
- Overflow text onto next row if width is reached.
- Clamp text within width/height bounds.

Rich text support: font loading, unicode, and kerning.  Bring alcohol.

# GUI + basic menu support

CRITICAL prerequisite: rendering at native screen resolution

Button system, color system

# Misc Issues
