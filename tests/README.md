# Tests

CTest registrations live with the target they verify. Add test sources here
when a tracer introduces a real invariant; do not add placeholder assertions.

The current fast suite covers exact known-byte PNG encoding, the accepted Tracer
0 baseline packet's files/hashes/verdict, the 60 Hz fixed-step accumulator,
window-state transitions, fatal project assertions, and the dummy-driver SDL
lifecycle. On a capable OpenGL 4.6 target it also covers context validation,
triangle and depth-tested cube framebuffer oracles, byte-identical repeated
normal and wireframe cube captures, and injected high-severity GL rejection.
The CMake wrappers distinguish required process behavior from a crash or a
missing diagnostic; the capture wrapper preserves failed outputs but removes
its temporary PNGs after a passing repeat comparison. The artifact-manifest
validator independently checks versioned Windows evidence packets' required
fields, retained files, and SHA-256 hashes. With accepted-review validation
enabled, it also requires exactly one Accept verdict plus an owner observation
and date. A nested CTest regression proves that the common failure regex rejects
project failure markers and ASan, LSan, and UBSan diagnostics even when output
also contains a configured pass marker.
