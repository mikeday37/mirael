# Script Node

The Script Node executes arbitrary user code configured in the UI.
Currently, only Lua 5.1 via LuaJIT is supported.

## Script Error Handling

A core premise of Mirael is that errors should be minimally disruptive.

If a new script fails to compile, a CoreStatus update is posted once with
the compiler error, and the Core is otherwise unaffected - it will continue
running the latest successfully compiled script.

If a compiled script has a runtime error, the behavior is configurable.  By default,
the node will simply display a red background, but will continue attempting to run
on every frame.  The red background is removed on subsequent successful runs.

It is thus possible for the node to show intermittent runtime errors
that it can automatically recover from (for example, continuously changing input
values that occasionally hit an unsupported range).  This gives maximum play to the
rest of the graph - parts of it only stop when they have to.

The runtime error handling options are:

- **Silent**: the background won't change in the case of error - this may be desirable
if it is deemed harmless by the user and the background is distracting.  The exact
error detail will still be available in the properties window when the node is
selected.

- **Visual**: the background will be red for any frame where there's a runtime error,
as described above.

- **AutoDisable**: when a runtime error is encountered, the background will go red and
the node will enter an AutoDisabled state.  In this state it will NOT attempt to re-run
on subsequent frames.  The AutoDisabled state can only be cleared by editing the script.
This gives the ability to catch an intermittent error to investigate it and resolve it,
but is less disruptive than a traditional breakpoint.  The rest of the graph can still
continue running, even if any Script nodes are AutoDisabled.

Regardless of the handling option, runtime errors only result in a CoreStatus update
if the script with the error is the latest, successfully compiled script.  Such an update is
NOT posted if the running script has been replaced with a script that doesn't compile.

The Enabled flag is a separate master switch.  No attempt is made to run scripts if
Enabled is false, but it will still attempt to compile new scripts.  They just
won't start attempts at running until Enabled is set to true.
