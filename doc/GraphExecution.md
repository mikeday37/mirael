# Graph Execution in Mirael

## Overview

Mirael is a high-performance, live visual dataflow system for rapid iteration on ideas and algorithms.

The chief design goal is to allow editing of dataflow graphs as they run,
without any need to stop or restart them, and with minimal impact on performance.

## Components

### Visual/Interactive Objects

- Execution is defined by directed, acyclic Graphs.
- Each Graph consists of Nodes and Links.
- Each Graph is independent and has its own settings to determine how often it executes.
- The Input and Output capabilities of Nodes are represented by Pins.
- Each Link connects an Output Pin from one Node to an Input Pin on another, defining the dataflow between them.
- One Execution of an entire Graph is called a Frame.

### Id Space

- Within a Graph: Nodes, Pins, Links all share the same 64-bit Id space.
- A given Id will only point to one type of object within the Graph.

### Internal Objects

#### Runners and Execution Plans

Each Graph has a Runner.  The Runner is created by the Graph on the UI thread, but then runs in its own thread.
When a Graph is loaded or topologically modified, it produces an Execution Plan and sends it to the Runner.
The Runner simply executes the Execution Plan in a loop, with an optional delay as determined by the
Graph's Run Rate Settings.

One Execution of a Plan results in a complete Frame of calculation.  Within a Frame, every Node executes once,
in topological order, to ensure that all required inputs are available before each output is calculated.

Before each Frame, the Runner checks for a new Execution Plan, and adopts it if it exists.

Within a Node, property edits and other user interactions (dragging a slider or clicking a button, etc.)
do NOT result in updated Execution Plans, as they do not modify the topology of the Graph.
Only addition or removal of Nodes or Links modifies the topology.

#### Nodes, Cores and Channels

Derived Nodes have a `createCore()` override.  Each Node Type must have its own NodeCore-derived type.

`createCore()` runs in the UI thread, but is responsible for creating and minimally setting up the Core that
will run in the Runner's thread.  Once `createCore()` returns, everything the returned Core does will be
controlled by the Runner.

For interactive Nodes, the derived Node / Core pair will have to communicate.  To maximize performance, such
communication must be non-blocking.  This is done by setting up a Channel that is held via `std::shared_ptr<>`
by both the Node and the Core.  The Channel is a custom type created within `createCore()` and suitable for
that Node Type's needs.  It is recommended to use the following types for subchannels within the Channel type:

- `std::atomic<>` for communicating latest values, where passing complete change history is not required.
- `moodycamel::ReaderWriterQueue` for communicating lossless streams of events when absolutely required.

#### Execution Plan Updates, Versions, and ResourceDeltas

Creation and adoption of Execution Plans must be fast to achieve Mirael's design goals.  Currently, this is
done via the following sequence:

- 1: Increment Plan Version.
- 2: Calculate a ResourceDelta and *queue* it for the Runner with the new Version.
- 3: Topologically Sort the Graph into a new Execution Plan.
- 4: Send the Execution Plan to the Runner with the new Version via *mailbox* (overwrite last if not adopted).

All of the above is non-blocking.

*Technical note:* It is vital the Runner threads sees the existence of a new Execution Plan strictly *after* the
corresponding ResourceDelta has been queued.  Internally, the Graph uses `std::memory_order_release` on step 4,
and the Runner uses `std::memory_order_acquire` when checking for a new plan, to ensure this memory ordering
is enforced.  This is essential for avoiding race conditions when Mirael is ported to non-x86 platforms that
do not enforce a Total Store Order in their memory model, such as ARM, PowerPC, etc.

A ResourceDelta consists of a version number and a list of the following events:
- Cores to delete
- Cores to add
- Output Pins to delete
- Output Pins to add

When the Runner sees a new Plan, it takes it, gets the Version number, then drains the queue of ResourceDeltas
until that queue is empty or it receives a Delta of higher Version than the new Plan.  It only applies new
Deltas of Version equal to or lower than the Plan.  If it receives a Delta of higher Version, it holds onto it
until finds a Plan of that Version or higher before applying it.

#### Execution Plan Contents

An Execution Plan contains what is needed to sequence execution of that version of the Graph:
- a list of Core Ids in topological order
- a map from Input Pin to the Ouput Pin(s) connected to it

#### Runner Resources

In order to Execute the Plan, the Runner must maintain the resources indicated by the Deltas:
- a map of Node Id to the Core that implements it
- a map of Output Pin Id to the value buffer that stores its data

Applying a Delta adds/removes the indicated objects.  Each Output Pin gets exactly one value buffer,
owned and managed by the Runner.

## Core Execution

It is vitally important that Node / Core pairs communicate *only* via their custom non-blocking channel.
It is also important to realize that Cores can outlive the Nodes that create them.  This will happen any
time a Node is deleted visually while an Execution Plan is mid-execution.

During a Frame of execution, each Core managed by the Runner for that version will have its `onFrame()`
called once.  Therein, it should perform only the following actions:

- read its Channel for any information coming from its Node, if applicable
- read the appropriate value buffers (using the Pin Map provided by the Plan) corresponding to its Input Pins
- perform its calculation
- write to the value buffers for each of its Output Pins
- write to its Channel for any information it needs to send back to its Node, if applicable

Again, all of this is non-blocking, and a Core doesn't need to know whether its Node still exists.  Any
outgoing info will be harmlessly freed during the next Plan update if needed.

## Execution Correctness vs. Design Priorities

While the above design guarantees toplogical correctness for each Frame of Graph execution, it does *not*
gaurantee that each node will have a configuration consistent with the Plan Version when that Plan is executed.
This is considered an acceptable situation given Mirael's design priorities, which are as follows:

Essential Priorites:
- High-performance real-time UI and graphics.
- Tweaking settings in real-time.
- Hot reload.
- Freedom to experiment without fearing "breaking the system".
- Eventual consistency - if the graph hasn't been modified, then the execution of the graph
should be consistent with its current layout and node configurations within 2 runs.

Preferred Priorities:
- The graph execution *should* be consistent with its current state where possible.

Relatively Unimportant:
- Absolute correctness for every frame while the graph is being modified.

Given the above, we've chosen to stick with a relatively simple and efficient communication and run model that
gaurantees consistency when modifications are not happening, while allowing realtime modifications which may
have unexpected side effects in edge cases.

It is important that Node Core developers are aware of this potential inconsistency and defensively implement
Cores accordingly.  For the currently envisioned use cases of the system, however, such considerations do not
appear to pose a signficant problem.

## Execution Lifecycle Summary

```
Graph topology edit
  -> increment Version
  -> queue ResourceDelta
  -> mailbox ExecutionPlan
    -> Runner adopts Plan
    -> Runner drains Deltas (up to Plan Version)
    -> Runner executes Frames
```
