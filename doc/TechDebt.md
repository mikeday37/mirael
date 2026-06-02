# Tech Debt

Here are some documented areas that could benefit from a fresh design and re-implementation.  Each area has an id such as `TD1` that
relevant code comments can use to point to this document.

## TD1: Graph-level Lua Init Scripts

See commit: <TBD>

The design and implementation of Lua Init Scripts was rushed.  This resulted in a new channel of comms from the Runner back to the Graph
(initScriptResult_), as well as the onLuaStateReset() virtual for NodeCore, which only ScriptCore uses.  ScriptCore uses it rather
awkwardly to trigger recompilation of the most recently received script in the latest lua state.

A better approach would involve a more standard way to get status/diagnostics from the runner, and perhaps recreating the runner
entirely if the lua environment needs to be recreated.  The latter may be necessary anyway if/when we support demoting runners to
ghost threads in the case of an infinite loop in a user script.  The demoted runner would have to be replaced for its graph to continue.