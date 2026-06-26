# Mirael

Mirael is a high-performance, **live** visual dataflow system.

- Watch on YouTube: [Mirael - Live Graphics Code Editing - Early Example](https://youtu.be/C6JOU6fxG5I)

## Purpose and Audience

The system is intended for technical artists and graphics developers who want to experiment with realtime graphics in an environment
where the time to iterate is effectively zero.  There's no need to cycle through *separate* phases of design, code, build, run, and
debug.  All such phases occur simultaneously.

## Project Status

This is a very early prototype - basic quality-of-life features like cut/copy/paste and undo/redo are not yet implemented.
Only Windows is currently supported.

However, the core premise is already proven:  The system is 'live' in the sense that you can freely modify the dataflow graph,
*including code within Script nodes*, at any time and see the results instantly (within the next frame, without dropping frames),
while the system continues running at full framerate.

## Core Technology Used

Mirael is built in C++20, using Dear ImGui with the GLFW and Vulkan backends.  LuaJIT is used for scripting.  VulkanMemoryAllocator is
used for GPU memory allocations.  Each Graph executes in its own thread, and every Node uses only lock-free methods for cross-thread
communication.

For licenses and attributions, see: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)

## Architecture and Documentation

See [`doc/GraphExecution.md`](doc/GraphExecution.md) for the core definitions, design priorities, and architectural decisions.
For additional details about Display and Script nodes, see [`doc/ImageHandling.md`](doc/ImageHandling.md) and
[`doc/ScriptNode.md`](doc/ScriptNode.md) respectively.

[`doc/TechDebt.md`](doc/TechDebt.md) tracks known areas for improvement.

The documents in [`doc/future_state`](doc/future_state) describe designs for upcoming enhancements:
- [`doc/future_state/InfiniteLoops.md`](doc/future_state/InfiniteLoops.md) discusses solutions to the problem of infinite loops
(or excessively long runtimes) within user code in Script Nodes.
- [`doc/future_state/SubObjects.md`](doc/future_state/SubObjects.md) discusses motivation and solution to allow *superficially* cyclic graphs.

## Getting Started

### Safety Considerations

***WARNING***

Mirael is for experienced technical artists and power users familiar with the dangers of running code live, as you change it.

The Lua environment is not fully sandboxed.  When using FFI to write to image buffers, you can easily corrupt memory and cause
crashes or undefined behavior if you make a mistake - This is a consequence of the deliberate choice to put the full power of the
environment at your fingertips.

I recommend wrapping FFI buffer writes in bounds-checking primitives (all the included examples do this), unless you're
sure you don't need that safety net and actually require the extra performance.

Use at your own risk.

### Prerequisites

- **Windows** - other platforms are not yet supported.
- [**Visual Studio 2026**](https://visualstudio.microsoft.com/downloads/) - Community edition is free and adequate.  VS 2022 may also work, but hasn't been tested.
- [**Vulkan SDK**](https://vulkan.lunarg.com/sdk/home)
- [**vcpkg**](https://learn.microsoft.com/en-us/vcpkg/get_started/overview) - Microsoft's package manager for C++.

### Environment Variables

Ensure that the following environment variables are set up correctly for your Vulkan SDK and vcpkg installations:
- `VULKAN_SDK`
- `VCPKG_ROOT`

### Anti-Virus False Positives

LuaJIT is sometimes flagged as a virus by antivirus software, both during the build and in the resulting executable.
This is a known false positive, and is not uncommon with just-in-time compiler systems that generate and execute
machine code at runtime.  Mirael links directly to the official v2.1 branch of LuaJIT as a submodule
(see [`.gitmodules`](.gitmodules)) - it is not malware.  You may need to add folder exclusions for the following
directories to build and run Mirael:

- `external\luajit`
- `out\build`

### Clone, Build and Run

Using the `x64 Native Tools Command Prompt for VS`:

```
git clone --recursive https://github.com/mikeday37/mirael.git

cd mirael

cmake --preset x64-debug
cmake --build out\build\x64-debug

cd out\build\x64-debug

mirael
```

## Using Mirael

### The Basics

- Double-click a Node Type in the Library to add it to the current Graph.
- Drag from an Output Pin to an Input Pin (or the other way around) to connect them with a Link.
- Select a Link or Node and hit Delete to delete it.
- Select a Node to view and edit its properties in the Properties window.
- With no Node selected, you can view/edit the Graph Properties instead.
- Use the mouse wheel to zoom in/out.
- Pan the Graph by dragging an empty spot with the right mouse button.
- Right click an empty spot and choose "Fit Content" to fit the entire Graph into view.

### Configuring the UI

All windows are dockable and multiple viewports are enabled.  Only the Project Explorer cannot be closed.

By default, the Graph takes up the most space, which you generally want if you're just watching it run.

However, when you're editing scripts, you will want to configure things so that the Properties window is
much larger.  An easy way to do so is to just drag the Properties tab to the right of the main window, and
dock it by itself on the right.  Or, drag it outside the window entirely, which can be especially handy
if you have multiple monitors.

Mirael stores UI configuration separately from Project/Graph files (via `imgui.ini`).

### Lua and Dataflow

Lua is a first-class citizen in Mirael.  Each Graph has its own Lua State that is shared among all nodes.
All Lua types are supported.

Note that due to the use of LuaJIT, Mirael only supports Lua 5.1 and
[LuaJIT's extensions to it](https://luajit.org/extensions.html).

For each Frame, every Node in a Graph is executed once in the topological order defined by the Links
between them.  Cyclic Graphs are invalid and are currently ignored - the system will always run the latest
non-cyclic version of the Graph.

Data values persist at Output Pins.  Links simply tell Mirael which Output Pin's value should be referenced by
a given Input Pin.  Currently, each Input Pin can only have one Link.

For Script Nodes, their Lua script is recompiled on every keystroke during modification, and run on every frame
by default.  See [`doc/ScriptNode.md`](doc/ScriptNode.md) for more details and options.

The Lua State is initialized by the Graph Init Script, which is currently only run once per Graph by default.
It can be changed and manually re-run, though doing so will reset the Lua State.  The Graph Init Script is
intended for setting up reusable global functions, and is the only script where globals can be easily modified.

Another option for function reuse is to output a function from a Script Node.  Such a function value can be
read by another Script Node and invoked within it like any other Lua function.  

For compatibility with the Display Node, image output requires the use of the FFI library and building a Lua
table with a specific schema.  See [`doc/ImageHandling.md`](doc/ImageHandling.md) for more details.

### Using the Script Node

Most of the power in Mirael currently resides in the flexibility of the Script Node.

You can freely add, rename, and delete Input and Output Pins simply by modifying the comma-delimited Pin lists
in the Node's Properties.  Input Pins are read-only.  Output Pins are read-write, and can be used both for output
and for persistent state.

Pins are referenced by position within the script itself, not by name, so renames do not affect execution.

Mirael currently provides the `input` and `output` globals for access to Pins within Script Nodes.
Here's an illustrative example:

```lua
-- script for a two input, one output node that simply adds the inputs:
local a,b = input() -- input as a function call reads all inputs
output(a + b)       -- output as a function call with at least one parameter sets all outputs

-- alternate implementation - use indexing rather than function calls to access one Pin at a time:
output[1] = input[1] + input[2]

-- a simple script which acts as a counter:
local v = output()  -- output as a function call with no parameters reads all outputs
v = (v or 0) + 1    -- outputs are nil until written, and persist across frames
output(v)
```

### A Note on Reference Types

Mirael does not perform deep copies of reference types as they pass from Node to Node - it simply
establishes or releases references to them.  This has consequences that can be surprising if not accounted for. 
For example, if a Node sends the same table value to multiple Nodes, and one of them modifies the table,
such modifications are visible to all such Nodes, regardless of Graph topology.

## Examples

Several ready-to-use Projects are found in the `examples` folder:

- `1-oldskool-effect-demo.mir` - the Graph shown in the video at the top of this document.
- `2-script-basics.mir` - demonstrates the scripts given in the *"Using the Script Node"* section.
- `3-plasma-effect.mir` - a more complex example of an old graphical effect.
- `4-cyan-bands.mir` - a pleasing variation of the original demo, discovered by chance.
