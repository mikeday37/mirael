# Script Node

The Script Node executes arbitrary user code configured in the UI.  Currently, only Lua via LuaJIT is supported.

## Script Error Handling

A core premise of Mirael is that errors should be minimally disruptive.

If a new script fails to compile, a CoreStatus update is posted once with
the compiler error, and the Core is otherwise unaffacted - it will continue
running the latest successfully compiled script, if any, if it didn't 
autoDisable itself.

If a compiled script has a runtime error, the core sets itself to autoDisabled,
but a CoreStatus update is posted only if the script with the error is the latest,
successfully compiled script.  Such an update is NOT posted if the running script
has been replaced with a script that doesn't compile.

A Core that is autoDisabled will not attempt to rerun the same script.

When a Core receives a new script that compiles, autoDisabled is reset and it
will attempt to run the new script.  If the new script fails, the runtime error
handling outlined above will immediately set it back to autoDisabled.

The enabled flag is a separate master switch.  No attempt is made to run scripts if
enabled is false, but it will still attempt to compile new scripts.  They just
won't start attempts at running until enabled is set to true.