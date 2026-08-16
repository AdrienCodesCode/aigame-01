# glad OpenGL loader

This directory contains the generated C loader for OpenGL 4.6 Core used by
Wide Eye. It is checked in so normal Linux and Windows builds do not require the
Python glad generator or Jinja at configure time.

## Provenance

- Generator: [glad v2.0.8](https://github.com/Dav1dde/glad/releases/tag/v2.0.8),
  commit `73db193f853e2ee079bf3ca8a64aa2eaf6459043`.
- Generator archive:
  `https://github.com/Dav1dde/glad/archive/refs/tags/v2.0.8.tar.gz`.
- Generator archive SHA-256:
  `44f06f9195427c7017f5028d0894f57eb216b0a8f7c4eda7ce883732aeb2d0fc`.
- Generated on: 2026-08-16.
- API: OpenGL 4.6 Core.
- Extensions: none beyond the selected core API.
- Loader option: disabled; SDL supplies `SDL_GL_GetProcAddress`.
- Generation mode: reproducible, using the specifications embedded in the
  pinned glad archive.

The exact generation command, run from the extracted glad archive, was:

```bash
python3 -m glad \
  --out-path /path/to/wide-eye/third_party/glad \
  --api gl:core=4.6 \
  --extensions='' \
  --reproducible \
  c
```

`cmake/WideEyeDependencies.cmake` verifies the SHA-256 of every generated input
before defining the loader target. A dependency update must regenerate all
three files, review the upstream license and generated SPDX expression, and
deliberately update those expected hashes.

## License

The generated files identify their license as
`(WTFPL OR CC0-1.0) AND Apache-2.0`. The retained upstream `LICENSE` records the
glad generator's MIT terms and the Khronos specification/header terms used to
produce them.
