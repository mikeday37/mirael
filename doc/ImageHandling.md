# Image Handling in Mirael

Creating dynamic graphical effects and changing the code to do so on the fly is a core use case for Mirael.

Accordingly, I expect image handling to become quite sophisticated in the future, to support such dynamism with extremely high performance
and reliability.

However, as this is a prototype, I'm starting small, simple, and adequate.

## Initial Image Handling Decisions

### Image Data Type

Images will be displayed via the Display node by simply passing it a single value (like it currently works with strings).
The Display node will treat that value as an image if:
- The Lua type is `table`
- The table has a `_tag = 'image'` field
- The table further has the following fields:
  - `w`: width in pixels
  - `h`: height in pixels
  - `buf`: pixel buffer managed via LuaJIT's FFI library, stored as an array of `rgba_t` which will be pre-defined as `struct {uint8_t r, g, b, a;}`
 
`buf` is tightly packed with no row padding, so each pixel `(x, y)` is at `buf[y * w + x]` (with FFI's 0-based indexing).

### Vulkan Details

The Display node will manage a triple buffer.  Each image in the buffer will be a Host-Visible Image created on the UI thread via
VulkanMemoryAllocator.  Those images will be used as textures with the following properties:
- `VK_FORMAT_R8G8B8A8_UNORM`
- `VK_IMAGE_TILING_LINEAR` (*)
- `VK_IMAGE_LAYOUT_GENERAL`

Triple buffering will allow non-blocking transfer of images between the UI and Runner threads for maximum speed.

(*) The Vulkan spec does not guarantee that Linear Tiling will always work for image sampling, so we will query for the capability at
startup, and fail with an appropriate error if not supported.  Fallback to another approach may be developed when we start porting to
other platforms.  This prototype currently targets only modern, high-end desktop Windows PCs, which nearly universally support the
properties listed above.

### Creating Images in Script Nodes

The default Graph Init Script will use the FFI library to setup the `rgba_t` cdecl, and store a global `newimage` constructor which
will automatically create an image lua table correctly, such as via this example code:

```lua
local myimage = newimage(320, 200) -- oldskool DOS vibes :D
```

### Synchronization and Limitations

All Vulkan calls will be handled on the UI thread.  The Display Core will simply ask for an available image of the right dimensions.
If no such is available, a request for a change of image dimensions will be posted, but the core will simply move on.  It won't wait,
and what it would have drawn with the new dimensions for that frame will be lost.

This means that there will be missed initial frames within individual Display Nodes on startup and whenever image dimensions change.
In fact, there is no gaurantee that images of continuously changing image dimensions will ever be displayed.  This is an acceptable
limitation for the first pass, as we expect image dimensions to change infrequently due to user input, not per frame (as with graph
topology changes).

It also means that the triple buffer managed by the Display node does not even exist until the Core posts a need for an image of
particular dimensions.  Multiple UI frames may pass before the Runner comes around again to give the Core the opportunity to fill
such a new image, during which time the UI will have nothing to display.  As such, when dimensions change, the UI may "flicker"
and display nothing for a few frames.  Again, this is considered an acceptable first pass design.