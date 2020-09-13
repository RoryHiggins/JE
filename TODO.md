
Correct scaling:
- TODO rethink- how would split screen be done?
	- client stores multiple viewports keyed by id.  fields: isActivated, depth, size, and window pos+size.  max 16
	- simulation maintans a table representing the viewport, and calls client.updateViewport() on the table every frame
	- client stores min, max viewport for each renderable
	- simulation optionally specifies renderViewport field to 

Editor thoughts: keyboard+mouse focused
- 2x or maybe even 1x scale
- Mouse wheel to zoom
- Space + left mouse to drag
- Command mode: on keyboard, press / to start a command
	Ordered by most recently used
	Optional 
- Filter: filtered out entites are translucent and cannot be selected
- Selection:
	hold left mouse button and drag to select area
	double-tap action to select all unfiltered entities
	tap shift to add to selection
	hold shift and drag to extend selection
	tap esc to deselect
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

