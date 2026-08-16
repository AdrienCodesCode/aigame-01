# How Far Can AI Push a From-Scratch Rendering Engine?

## Original question (verbatim)

> How far do you think I can push my rendering engine (the one I’m building from scratch) with today’s ai capabilities , I want to get to something as pretty and polished as the best visuals of Aaa games, well let’s say 80% there. It’s all software , nanites, ue lumen are just code, although probably internal to UE only… but there must be if not cutting edge publically available research, at least something fairly recent and advanced to help ai ? 
> I’m ready to iterate for months to help the ai come up with advanced algorithms and optimisations and pipelines etc to push my graphics to the edge..
> I mean we got agent solving mathematical problems 100 years old.. compared to that, making an engine that renders fast and well , given that graphics have been pretty good for years now. Shouldn’t be impossible ?

## Response

My blunt answer: **yes, you can push it shockingly far**. For your particular game, I think “80% of the perceived visual impact of AAA” is plausible.

But the winnable target is:

> “Make one stylized voxel herding world look AAA-adjacent during normal third-person gameplay.”

Not:

> “Rebuild 80% of Unreal Engine.”

Those are completely different projects.

| Target | My honest judgement |
|---|---|
| Impressive screenshots or a short trailer | Very plausible |
| A polished vertical slice in one valley/biome | Plausible, though ambitious |
| Consistent AAA-adjacent quality throughout a narrowly scoped game | Possible with sustained iteration |
| 60 fps, every camera angle, dynamic weather, multiple GPU vendors, no popping or ghosting | Likely multi-year territory |
| A general Nanite/Lumen competitor supporting arbitrary games | Not realistic in months |

As a rough judgement—not a promise—**6–12 months of serious work could produce a vertical slice people assume came from a major engine**. Twelve to twenty-four months could potentially make that quality consistent during normal gameplay, provided you keep the renderer extremely specialized.

## Your “it’s all software” argument is directionally right

A remarkable amount of the knowledge is public:

| Problem | Public starting point | What your engine actually needs |
|---|---|---|
| Virtualized geometry | Epic’s detailed [Nanite technical presentation](https://advances.realtimerendering.com/s2021/index.html), [meshoptimizer](https://github.com/zeux/meshoptimizer) | Chunk/meshlet hierarchy, screen-space-error LOD, GPU culling and streaming—not full Nanite |
| Voxel terrain LOD | [Transvoxel](https://transvoxel.org/) | Crack-free transitions between voxel chunk resolutions |
| Dynamic GI | AMD’s open, MIT-licensed [Brixelizer GI](https://gpuopen.com/fidelityfx-brixelizer/) | Particularly relevant because it uses sparse distance fields and supports Vulkan |
| Advanced ray-traced lighting | [ReSTIR GI](https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing), Vulkan ray tracing, RTXGI/NRD | Optional high-end path after the rasterized foundation works |
| Temporal reconstruction and effects | [AMD FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK) | TAA/upscaling, denoising, AO, reflections and post-processing |
| PBR/material foundation | Google’s [Filament PBR reference](https://google.github.io/filament/Filament.md.html) | A production-quality reference for BRDFs, exposure, tone mapping and color management |

Nanite’s architecture is described publicly from mesh import through hierarchy construction, streaming, decompression, culling, rasterization and shading. Lumen’s public presentations discuss distance-field tracing, surface caches, radiance caches, hardware ray tracing, final gathering and reflections. The information shortage is **not** the main blocker.

The missing part is thousands of production details: temporal instability, memory pressure, streaming failures, material incompatibilities, camera cuts, foliage shimmer, driver bugs, debugging tools and years of accumulated edge-case fixes.

Also, don’t ask AI to invent a revolutionary algorithm first. The better sequence is:

1. Implement the known reference technique.
2. Match reference output.
3. Instrument and profile it.
4. Specialize it for your voxel world.
5. Only then experiment with new algorithms.

## Why the mathematics comparison is slightly misleading

A theorem has a precise statement and often a crisp verifier: the proof checks or it does not.

“Looks AAA” simultaneously means:

- beautiful;
- temporally stable;
- fast;
- memory-efficient;
- art-directable;
- robust during streaming;
- correct on different GPUs;
- compatible with every other rendering pass.

Current agent benchmarks predominantly test clean, self-contained tasks with automatic success criteria. METR explicitly warns that those results do not directly predict months-long, messy collaborative projects, and that agent performance drops when output is judged holistically rather than by a narrow grader. [METR’s current methodology](https://metr.org/time-horizons/)

But there is strong reason for optimism. OpenAI reports that GPT‑5.6 in Codex autonomously rewrote and optimized production kernels, with broader agent-assisted optimizations contributing to a 20% end-to-end serving-cost reduction. Crucially, they also invested heavily in verification tooling to ensure those kernels were actually correct. [OpenAI’s GPT‑5.6 engineering report](https://openai.com/index/gpt-5-6-frontier-intelligence-efficiency/)

That is exactly the pattern you should copy:

> AI can climb the optimization hill extremely well—but you must build the hill, the measuring equipment and the guardrails.

## Turn your engine into an AI-readable graphics laboratory

The most important system may not be GI or meshlets. It may be the evaluation harness:

- Deterministic camera routes, fixed seeds, exposure and scene state.
- Automatic screenshot and short-video capture.
- Outputs for depth, normals, motion vectors, shadow masks, GI, history rejection, LOD and overdraw.
- A slow offline reference renderer for selected frames.
- Per-pass GPU timing, VRAM, bandwidth, draw/dispatch and streaming statistics.
- Golden-image regression tests using something like NVIDIA’s open [HDR/LDR FLIP metric](https://github.com/NVlabs/flip).
- Separate unseen “holdout” scenes so the agent cannot optimize only your showcase camera.
- RenderDoc/Nsight/Radeon captures that the agent can inspect.

AMD even released an open MCP integration in July 2026 that lets LLM agents analyze GPU crash dumps, resource timelines and shader evidence before proposing source fixes—graphics tooling itself is becoming agent-readable. [AMD RGD MCP workflow](https://gpuopen.com/learn/post-mortem-gpu-crash-debugging-with-llms/)

Then you can give Codex specifications such as:

> Implement stabilized sun shadows. On the deterministic valley route, remain within the assigned GPU and memory budgets, produce no visible cascade seam, and pass the vegetation-motion and rapid-camera tests. Include debug views and before/after captures.

That is vastly more effective than “make the shadows AAA.”

## For your herding engine, don’t clone Nanite or Lumen

Your data is already specialized.

- Terrain has a voxel/chunk hierarchy, so exploit that instead of supporting arbitrary film-resolution imported meshes.
- Use GPU-driven vegetation instancing, HZB occlusion and distance LOD.
- Use ordinary skeletal/animation LODs for the dog and nearby sheep, simplified animation farther away and impostors at extreme distance.
- Your voxel representation can help with broad indirect lighting, skylight occlusion and distance-field tracing.
- A controlled third-person camera gives you a known height, FOV and close-up range, allowing aggressive optimizations and carefully selected detail distances.

For this game, the largest visual wins will probably come from:

1. Physically coherent sunlight, sky and exposure.
2. Stable, high-quality shadows and contact grounding.
3. Excellent terrain materials with macro/micro variation.
4. Dense grass, wind and believable vegetation distribution.
5. Aerial perspective, fog, clouds and volumetric sunlight.
6. Sheep and dog animation quality.
7. Temporal stability and polished color grading.
8. GI and reflections after those foundations work.

A perfect GI algorithm cannot rescue sliding sheep feet, repetitive procedural rocks or unconvincing grass movement. Because you are avoiding authored assets, your procedural generators effectively become your asset-production department.

## One architectural warning: OpenGL

OpenGL does not impose a low visual-quality ceiling. You can build an excellent GPU-driven rasterized voxel renderer with compute shaders, indirect drawing and sparse resources.

But modern cross-vendor mesh shading and hardware ray tracing are standardized in Vulkan, while OpenGL’s mesh-shader route is largely vendor-extension territory. Vulkan exposes cross-vendor [mesh shaders](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_mesh_shader.html) and [hardware ray tracing](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_ray_tracing_pipeline.html), and most current high-end SDK integrations prioritize Vulkan/DX12.

I wouldn’t blindly rewrite everything now. I would:

- keep OpenGL as the working/reference backend;
- make render resources and passes API-neutral;
- build a modest render graph;
- add Vulkan when a chosen advanced feature genuinely requires it.

If the engine is still early and hardware RT/Brixelizer/mesh shaders are definitely part of the destination, starting that abstraction now will save pain later.

So no, your ambition is not impossible or ridiculous. **One person plus frontier agents can now plausibly build rendering subsystems that previously demanded a small specialist team.** Your physics background is also genuinely useful for reading papers, checking assumptions and designing experiments.

The winning ambition is not to out-Unreal Unreal. It is to build the most ruthlessly specialized renderer possible for one beautiful voxel herding landscape. That could absolutely reach the point where ordinary players cannot believe it was built essentially by one person and AI.
