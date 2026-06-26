# Handling Infinite Loops in Scripts

***This document is Under Construction and describes potential future state.***

Since Mirael runs user-written code in Script Nodes, it's possible for the user to cause an infinite loop on the Runner.  Ideally, this
should be recoverable.

However, guardrails are not free, and their cost can be magnified in high-performance systems.  Where should Mirael strike the balance
between safety and performance, and how?

## Behavior with No Guardrails

Without any protection against infinite loops, entering the following into a Script Node causes the corresponding Runner to enter a
permanent state of zero FPS:

```lua
while true do end
```

Even in this state, the UI, of course, appears largely unaffected and continues operating at full-framerate.  You can even delete the
offending Script node, and add/remove/modify any Node or Link.  Other Graphs continue operating normally.  However, the following
consequences persist until the process is terminated:

- Script updates and calculation no longer works for any Node (of any type) in the affected Graph.
- If you attempt to delete the Graph, or exit Mirael, the UI hangs.

Let's call this "Runner Lock."

## Ideal Behavior

Runner Lock should be reversible.

Ideally, coding an infinite loop should be nearly as harmless as creating a circular graph.  It is correct for a Graph with an infinite
loop in a Script to run at zero FPS and to prevent all other Nodes in that Graph from calculating, but it is unacceptable for that state to
be permanent.  Infinite loops should be detected and highlighted as a problem for the user to resolve when they choose to.  The user should
be able to cancel execution of any Script (whether it has an infinite loop or not, or is simply long-running).

Also, the user should always be able to delete a Graph no matter its state, and exit Mirael without having to terminate the process.

It should be impossible to hang the UI.

## Potential Solutions

### Lua Debug Hook

The safest, most direct solution is to always set up a Lua Debug Hook in the Script Environment.  The hook can check the Runner's
stop token and raise an error if a stop was requested, instantly terminating any running Script.  This could be extended to use another
signal rather than the stop token, so the thread could simply resume after the Script errors out.

However, hooks don't work with compiled Lua, so JIT would have to be disabled, which would significantly lower the performance ceiling.

### Ghost Threads

Another option is to support "demoting" a Runner to a "Ghost Thread".  As a Ghost, the Runner would be completely detached from
the Graph UI and replaced with a fresh Runner.  The means of accomplishing this need not depend on a Locked Runner's cooperation.  The
Graph could simply move the Runner pointer to a graveyard, create a new Runner, and tell all Nodes to create new Cores on the new Runner.
They could then harmlessly release their shared pointers to the original custom channels they use to talk with their old Cores.  From the
perspective of the old Cores, it would be like their Nodes were deleted, which is already supported - Cores can always outlive Nodes.

This solution lowers performance only when there's actually a Ghost - the Locked Runner would keep eating CPU cycles (and potentially
affecting caches and memory bandwidth, etc.) until application exit.  This can be somewhat mitigated by lowering the priority of Ghost
Threads.  During application exit, Mirael could timeout while waiting for Ghost threads to join, calling std::quick_exit() to force
the exit.

### Yield Command

## Tradeoffs and Acceptable Behavior

### Minimal Acceptable Behavior

### Best of All Worlds, for Power Users
