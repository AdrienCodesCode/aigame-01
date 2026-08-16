# ADR 0002: Initial voxel chunk edge length

**Status:** Accepted for Tracer 1
**Date:** 2026-08-16
**Decision owner:** Project owner through the approved roadmap measurement gate

## Goal

Select the first production chunk edge for the bounded voxel paddock without
mistaking a synthetic fixture for a finished storage or meshing design.

## Decision

Use **16 cells per axis (16³)** for the first chunk storage and rebuild queue.
Keep coordinate conversion caller-configurable so tests and diagnostic tools can
still exercise other positive edges. Revisit the storage edge only if real
chunk metadata, meshing, upload, draw-submission, or target-hardware evidence
shows that the smaller edge prevents a budget from passing.

This selects an initial engineering constant; it is not a claim that 16³ is
universally optimal.

## Measurement contract

The fixture in [`chunk_size_comparison.cpp`](../../tests/chunk_size_comparison.cpp)
partitions the same deterministic 32³ field as either eight 16³ chunks or one
32³ chunk. It reports:

- logical and reserved cell bytes for a one-byte occupancy model;
- actual fixture control-object bytes, excluding allocator metadata and
  fragmentation;
- an equivalent full-field exposed-face scan;
- an interior edit, which affects one chunk under either candidate; and
- an edit at `x=16`, which affects two 16³ chunks but remains inside one 32³
  chunk.

The scan is a rebuild-cost proxy, not a production mesher. Storage and timing
samples are created before the measured loops, and timing is reported without a
machine-dependent pass threshold. CTest instead requires exact cell counts,
dirty-chunk counts, and equivalent full-field occupied-cell and visible-face
results.

The decision rule established before measurement was to prefer 16³ if it
materially reduced edit-local scanned work, kept equal-world fixture memory
essentially flat, and did not make the full rebuild at least 25% slower. A
contradictory result would have deferred selection.

## Observed results

On 2026-08-16, the release fixture ran under GCC 13.3.0 in Ubuntu 24.04.4 WSL2
on an Intel Core i9-8950HK. Each timing reports the median of 21 samples; each
sample repeats its workload enough times to scan at least 1,048,576 cells.

| Candidate and workload | Chunks rebuilt | Cells scanned | Median |
| --- | ---: | ---: | ---: |
| 16³ full 32³ field | 8 | 32,768 | 1,740,255 ns |
| 32³ full 32³ field | 1 | 32,768 | 1,704,272 ns |
| 16³ interior edit | 1 | 4,096 | 176,301 ns |
| 32³ interior edit | 1 | 32,768 | 1,605,666 ns |
| 16³ boundary edit | 2 | 8,192 | 399,855 ns |
| 32³ boundary edit | 1 | 32,768 | 1,640,660 ns |

Both candidates stored 32,768 logical cell bytes. Including reserved capacity
and the fixture's control objects, 16³ used 33,120 modeled bytes and 32³ used
32,840: a 280-byte, 0.85% difference. Both full rebuilds produced 26,211
occupied cells and 35,462 visible faces. The 16³ full rebuild median was 2.1%
slower, below the predefined 25% guard, while its exact edit-local work was 8×
smaller for the interior case and 4× smaller for the boundary case.

WSL Clang 18.1.3 development and ASan/UBSan builds plus the GCC 13.3.0 release
build each passed all 10 CTests. Native Windows 11/MSVC 19.44.35228.0 passed all
17 CTests, including the comparison invariant, and reproduced both accepted
Tracer 0 capture hashes with zero high-severity OpenGL messages. The ignored
[measurement manifest](../../artifacts/phase2/2026-08-16/chunk-size-comparison-wsl-gcc13-release-manifest.json)
records commands, source hashes, exact minima/medians/maxima, platform data,
limitations, and the raw-log hash. The matching ignored
[Windows packet](../../artifacts/phase1/2026-08-16/windows-cube-smoke-013203819/manifest.json)
records the native build and regression evidence.

## Inference and limitations

The meaningful signal is rebuild granularity, not a broad speed claim: one
local edit touches substantially fewer cells with 16³, while the equal-world
fixture-memory cost is negligible relative to the provisional 512 MiB Tracer 2
cap. The measured full-rebuild difference is small enough that it does not
outweigh that granularity benefit.

The fixture does not measure production palette storage, mesh buffers, allocator
overhead, RSS, worker scheduling, GPU uploads, draw calls, or the provisional
Low hardware profile. Eight 16³ chunks also imply more metadata and potential
submission/queue work than one 32³ chunk. Those costs must be measured when the
real systems exist; they are not silently assumed away here.

## Consequences

- The next voxel-storage outcome can define palette/material IDs and an explicit
  empty block around a 16³ production chunk.
- Coordinate conversion remains generic rather than embedding the selected
  storage constant.
- Dirty-region tracking and meshing remain separate roadmap outcomes.
- A future edge change requires comparable real storage, meshing, upload, and
  target-hardware evidence plus an explicit superseding decision.
