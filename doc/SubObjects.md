# Sub Objects in Mirael

***Not current state.  This is under consideration for future iterations of the engine.***

Internal/Virtual Objects:
- Each Node has one to many SubNodes.
- For each Node, by Default:
  - all Pins in the Node are connected to the Node's default SubNode, which has the same Id as the Node itself.
  - the Node contains no SubLinks.
- A Node can add one or more SubNodes, which each get their own NodeId.
- A Node can add one or more SubLinks between SubNodes, which each gets their own LinkId.
- SubNodes and SubLinks can divide a node into separate points of execution in a Graph, and determine their dependency order, if any.

## Why SubNodes?

Graphs are more useful if:
- A: outputs from one Frame can be inputs to the next Frame.
- B: results of calculation can be rendered in the same control as inputs to that calculation

SubNodes allow us to support case A via "Repeater" nodes, which have an Input Pin that is sequenced *after* its Output Pin.
Internally, the Repeater saves that input to use as its output on the next Frame.  For example, we could show a simple incrementing
counter via this graph:

```
                  Value (set to 1)
                    |
                    v
in.Repeater.out -> Add --> Display
 ^                  | 
 |                  |
 \------------------/

```

Without subnodes, this would be a cyclic graph, which is not allowed.  But internally, the Repeater looks like this:

```
[SubNode 1: .out]
      |
      v
[SubNode 2: .in]
```

The resulting internal graph, taking sublinks into account, is acyclic:

```
                  Value (set to 1)
                    |
                    v
   Repeater.out -> Add --> Display
       |            | 
       v            |
   Repeater.in <----/

```

SubNodes also allow us to support case B elegantly.  For example, we could have an IFS Editor Node,
which allows the user to visually define points and segments and outputs their data via a Pin.
The user can define connect their own logic to calculate matrices from that output, and use them to
render the fractal using whatever method they want.  The rendering could be fed back into the Node
as the editor's background image.

The resulting Node gives a better user experience, because the definition and the result are displayed together.
If the result is calculated fast enough, and the Mirael engine times things properly, there won't even be any lag between
dragging control points and the resulting fractal rendering.
