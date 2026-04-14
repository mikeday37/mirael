# Mirael

*Prototype for Silk - a live visual dataflow environment.*

## Lexicon

- Node - a unit of work within a graph.
- Edge - a relation between two nodes (may be the same node in some cases) - usually a relation that transfers data in one direction.
- Pin - a part of a Node to which an Edge can connect.
- Space - an infinite 2D canvas in which Nodes and Edges can be placed.
- SpaceType - a type of Space, which determines which types of Nodes and Edges can be added, and how to interpret and execute the resulting graph.
- Project - a collection of related spaces.
- View - a window which shows a portion of a graph for viewing and/or user interaction.
- Workspace - a collection of loaded projects and display settings for open windows and Views.

## Initial Scope

- Only one SpaceType = SimpleFlow, only one Edge type = DataFlow, data crosses one edge per frame.
- Simple Node visuals:  Just a box with input pins on the left and output pins on the right.
- Crucial Fundamentals:
    - Vulkan for graphics
    - GLFW3 for cross-platform basics
    - ImGui for UI
    - Multi-Monitor Support (using docking branch of ImGui with multi-viewport support)
    - Python Scripting, node behavior and UI can be defined entirely in Python
    - Saving/loading of Projects and Workspaces
    