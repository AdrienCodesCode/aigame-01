# Accepted visual baselines

Files below this directory are checked-in evidence promoted only after an
explicit owner Accept verdict. Do not overwrite an accepted packet, edit its
manifest hashes, or loosen its validator to normalize a changed result without
a new owner-reviewed candidate.

The accepted baselines are:

- The
  [Tracer 0 `voxel_cube_smoke` v1 reference-machine packet](tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md),
  which retains normal and diagnostic PNGs together with manifest-linked
  configuration, state, source hashes, run log, and owner review.
- The
  [Tracer 1 `handcrafted_paddock` v1 blockout packet](tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/review.md),
  which retains the accepted 960×540 normal frame, capture-time scene metrics,
  source hashes, limitations, and owner review. Its intentionally deferred
  matching debug view remains an unchecked Phase 2 outcome.

Registered CTests check each retained artifact hash and recorded verdict. Exact
pixel identity is a reference-machine property, not an assumed cross-GPU
guarantee.

The root [`.gitattributes`](../../.gitattributes) disables text conversion for
this directory. Accepted packets are hash-addressed evidence, so their original
bytes—including Windows line endings and captured log spacing—must survive
Linux and Windows checkouts unchanged.
