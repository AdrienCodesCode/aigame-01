# Accepted visual baselines

Files below this directory are checked-in evidence promoted only after an
explicit owner Accept verdict. Do not overwrite an accepted packet, edit its
manifest hashes, or loosen its validator to normalize a changed result without
a new owner-reviewed candidate.

The current baseline is the
[Tracer 0 `voxel_cube_smoke` v1 reference-machine packet](tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md).
It retains the normal and diagnostic PNGs together with the manifest-linked
configuration, state, source hashes, run log, and owner review. Its registered
CTest checks every retained artifact hash and the recorded verdict. Exact pixel
identity is a reference-machine property, not an assumed cross-GPU guarantee.

The root [`.gitattributes`](../../.gitattributes) disables text conversion for
this directory. Accepted packets are hash-addressed evidence, so their original
bytes—including Windows line endings and captured log spacing—must survive
Linux and Windows checkouts unchanged.
