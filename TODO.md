

# Misc Issues


# Render targets

Client: add support for triggering a viewport draw, with the given viewport

Engine: add support for multiple game cameras
- Expose the current camera in the render event
- replace camera.cameraTarget with a CameraFollow component
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
