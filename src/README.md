# Source boundaries

Phase 1 begins with only the ownership boundaries required by the accepted
architecture. Add APIs inside a boundary when a tracer needs them, not to fill
out a speculative engine framework.

- `core`: platform-independent monotonic time, fixed-step scheduling, logging,
  and assertions.
- `platform`: process entry, SDL lifecycle, native windowing, and input.
- `render`: OpenGL resources and presentation of immutable render state.
- `voxel`: voxel storage, coordinate conversion, generation, and meshing.
- `game`: authoritative fixed-tick game rules and render snapshots.

The executable entry point starts in `platform`, whose current window-state
reducer owns drawable resize, minimize/restore, focus, and close transitions.
The `render` boundary owns the Phase 1 triangle and perspective voxel-cube
pipelines, including color/depth framebuffer oracles, top-left RGBA8 readback,
and deterministic PNG encoding. The `platform` entry point exposes capture only
through the named cube smoke and owns the caller-supplied output path. The
`voxel` and `game` boundaries remain empty until a tracer needs their owned
behavior.
