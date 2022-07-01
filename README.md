# Doom Node Visualizer

![image](https://user-images.githubusercontent.com/554740/176973155-5e56e6d4-00d0-4aaf-83b4-bc79c52d05af.png)

This is a simple application which can load wad files made for DooM and display the BSP information and allow a user to navigate down the BSP tree.

## Building

The project is configured to look for SDL within your user directory. Unzip SDL2 to the following directory and everything should just work:

`$(USERPROFILE)\lib\sdl2`

On windows USERPROFILE is typically `C:\Users\<Your user name>\`

## Running

From the command line:

`doom-node-visualizer <path-to-wad>`

From windows:

Drag the wad you want to view into the program.

## Navigation

When viewing a map, the root split will start out displayed as a green line. Move the mouse to either side of the split to highlight the child nodes. Click the left mouse button to select that child node. The view will zoom in to fit the new node and its children in view.

### Keyboard shortcuts

Escape - Return to the root node of the map
PgDown - Cycle to the next map in the wad
PgUp   - Cycle to the previous map in the wad
