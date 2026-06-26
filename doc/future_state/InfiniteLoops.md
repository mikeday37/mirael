# Handling Infinite Loops in Scripts

***Not current state.  This is under consideration for future enhancements.***

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

Even in this state, the UI, of course, appears largely unaffected and continues operating at full framerate.  You can even delete the
offending Script Node, and add/remove/modify any Node or Link.  Other Graphs continue operating normally.  However, the following
consequences persist until the process is terminated:

- Script updates and calculations no longer work for any Node (of any type) in the affected Graph.
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
Threads.  During application exit, Mirael could timeout while waiting for Ghost Threads to join, calling std::quick_exit() to force
the exit.

### Yield Command

A third option is to let the user proactively protect loops that they anticipate may be at risk of accidentally running too long or
infinitely, by introducing a `yield` command they can manually add.  Like the debug hook, `yield` would check the stop token or another
signal, raising an error if the current Script needs to stop.

On the plus side, this would work even with compiled Lua, JIT could remain enabled, and the user would have control over the performance
impact by deciding where to place `yield` commands.  On the other hand, this will be of no help to a user who forgets or simply decides
not to use the `yield` command.

### Kicking

Above, we mentioned checking the stop token or "another signal."  Let's call that other signal the Kick signal.  Kicking a Script Node
causes it to raise a `kicked` error the next time it enters a debug hook or the `yield` command.  This allows the user to Kick the node
out of a long-running operation or infinite loop *if* the user has corrected the problem in the Script, *and* the Script is properly
instrumented (either through the option to enable Debug Hooks or explicit use of the `yield` command).

Note that if the Script has no `yield` commands, or began execution before enabling Debug Hooks, Kicking will have no effect.

## Minimal Acceptable Behavior

A minimum acceptable solution would provide these guarantees:
- long-running scripts can't ever freeze the UI
- the user can always edit the Script at fault, save their work, and continue after restarting the process

## Best of All Worlds, for Power Users

We can implement all four of the above solutions:
- add a Graph-level option to enable Lua Debug Hooks (resets the Lua state upon changing it), but leave it off by default (*)
- add a manual option to demote a Runner to a Ghost Thread and replace it (and a UI to see the status of those Ghost Threads, etc.)
- add a new global: the `yield` command
- add a Kick button to the Graph properties

This gives the user the ability to decide what risk-vs-performance profile they want to work within, and to change that decision at will.

(*) Why leave Debug Hooks off by default?  With Ghost Threads implemented, the worst case is a recoverable and probably mild degradation of
performance.  No loss of data, no UI freeze.  It's not severe enough to warrant lowering the *default* performance ceiling.  After
experiencing the need to demote a Runner to a Ghost, the user can always enable Debug Hooks if they want help iterating on problematic code
until they're confident enough to go back to max performance again.
