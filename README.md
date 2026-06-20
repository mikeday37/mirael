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

For links, see: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)

## Architecture and Documentation

See [`doc/GraphExecution.md`](doc/GraphExecution.md) for the core definitions, design priorities, and architectural decisions.
For additional details about Display and Script nodes, see [`doc/ImageHandling.md`](doc/ImageHandling.md) and
[`doc/ScriptNode.md`](doc/ScriptNode.md) respectively.

[`doc/TechDebt.md`](doc/TechDebt.md) tracks known areas for improvement.

[`doc/SubObjects.md`](doc/SubObjects.md) describes a design for an upcoming enhancement to Graph Execution.

## Getting Started

```
git clone --recursive https://github.com/mikeday37/mirael.git
echo TBD
```