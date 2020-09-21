

Render targets
-------------------

Add viewports support, particularly for the GUI.  Planned interface (by example):
```
	camera = {["x1"] = ..., ["y1"] = ..., ["x2"] = ..., ["y2"] = ...,}
	client.drawClear()

	self:broadcast("onSimulationDraw")
	client.draw({"cameras": {camera}})  -- splits window evenly between provided viewports.  renders at specified size before drawing to 

	self:broadcast("onSimulationDrawGui")
	client.draw()  -- defaults to full size at window resolution
```


Modularization
--------------
Add support for a command-line argument to specify the entrypoint

Move src/game to games/game/.  Add a games/examples/

Multiple spritesheets - how?  TODO thoughts

Move x/y/w/h handling into a Bounds2d system


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

Font loading and unicode.  Bring alcohol.


Misc Issues
-----------

- Releasing with a specified game does not change the client's default target
- No docs example of running a game headless (no client) - add a makefile target?

