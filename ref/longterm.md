# Long-term engine ideation transcript

**Status: unverified external ideation.** This is a pasted conversation with an
external AI model (recorded August 2026; exact date unrecorded), retained under
the `ref/` ideation rule in [`AGENTS.md`](../AGENTS.md). Nothing below is
project evidence: the named third-party experiments, model releases, benchmark
figures, and capability trends are unverified claims that this repository has
not reproduced, and no schedule or capability statement here is a commitment.
The authoritative direction remains [`ROADMAP.md`](../ROADMAP.md) and the
approved design and decision documents.

---

me: Ai are getting crazy good and fast Capabilities , long horizon runs etc.. feels we are at an inflexion point, exponential is coming soon if not now, competition is fierce , model releases are frequent. Comparing what was possible coding not even a year ago with agents an now feels absolutly unreal. So in 1 year from now…. My idea is instead of vibe coding “short” small scale apps, starting to code something like Unity , photoshop now, might feel like a mountain now for ais today, but it’ll be a hill in a few months, and meesily bump in a couple years. I guarantee some people are thinking long term, and have projects to iterate for a year , and release huge softwares comparable to adobe style software etc.. I saw a guy code his own CAD software.. I might do a voxel game with this engine from scratch, but extrapolate with me like I said. What could this engine end up as if I iterate daily for a year, assuming the rate of progress of ai continues to accelerate ?

gpt:
Yes—I think you’re seeing something real.

It may not yet be a provable “runaway exponential,” but we’re crossing practical reliability thresholds. A model becoming 20% better can suddenly transform a task from “fails constantly” to “can run overnight.” That creates an economic inflection even if the underlying capability curve is smoother.

The evidence is already slightly absurd:

In February 2026, Codex ran for roughly 25 hours from a blank repository and produced a 30,000-line design application while following a spec, testing and repairing failures. Official OpenAI documentation
Anthropic ran 16 agents across nearly 2,000 sessions and produced a 100,000-line Rust C compiler capable of compiling the Linux kernel on three architectures. It cost nearly $20,000 and still had architectural and regression problems—but this was achieved in about two weeks, not years. Anthropic’s compiler experiment
Google released Gemini 3.7 Flash only three weeks after 3.6, with a reported long-horizon software-engineering jump from 49.0% to 65.3% on one evaluation. Google’s announcement

So your intuition—“begin climbing the mountain now because future models will flatten it beneath me”—is strategically sound.

What one year could realistically produce

Assuming you work on it seriously every day, keep the project coherent, and AI progress continues:

Scenario Likely result after one year
AI barely improves from today A sophisticated custom voxel engine and a visually polished herding game
Recent trend continues A focused commercial-quality voxel engine, editor and procedural-world tool
Agent reliability improves faster than trend Something resembling a small, specialized Unity/Houdini for voxel and procedural games
Extreme acceleration plus significant multi-agent compute A broad creator platform potentially usable by other developers, though still less mature than Unity

METR measured approximately a 6–7 month historical doubling in the length of software tasks frontier agents can complete. If that particular trend continued for twelve months, it would imply roughly a 3–4× increase in reliable task horizon, not 100×. But crossing thresholds matters enormously: today’s barely achievable subsystem may become routine next year. METR also cautions that current measurements above about 16 human-hours are unreliable and cannot safely be extrapolated to year-long collaborative projects. METR’s current results, METR’s extrapolation warning

The strongest plausible end state

Your engine could evolve into something like:

A Vulkan-based, AI-native voxel world engine combining a Teardown-like runtime, a Houdini-lite procedural system and a focused Unity-like editor.

Potentially containing:

Advanced GPU-driven voxel and mesh rendering.
Dynamic terrain creation and destruction.
Procedural biomes, vegetation, rocks, roads and settlements.
Global illumination, volumetrics, weather and physically based atmosphere.
Large-scale animal/crowd simulation.
Animation graphs, procedural gait, IK and terrain-aware foot placement.
Physics, navigation, audio, input, saves and game-state systems.
An integrated scene editor with gizmos, inspectors, undo/redo and profiling.
A procedural node graph for world generation and materials.
Hot reload or a scripting layer.
Automated packaging and game builds.
An AI command layer: “create a wet Scottish valley,” “profile this camera path,” or “reduce vegetation cost by 25% without visible loss.”
Your completed herding game as its first flagship demonstration.

That wouldn’t be “Unity” in breadth. But it could be a legitimate engine that is dramatically better than Unity at one particular thing: creating beautiful procedural voxel environments.

That specialization is your shortcut.

The important distinction: feature parity versus product parity

AI may soon reproduce the obvious functionality of Photoshop, CAD software or Unity surprisingly quickly.

But Adobe-scale maturity includes:

Perfect file compatibility.
Crash recovery.
Huge undo histories.
Color-management and print accuracy.
Accessibility.
Plugin ecosystems.
Thousands of strange workflows.
Twenty years of bug reports.
Documentation, tutorials and user expectations.
Reliability on countless hardware configurations.

Similarly, your engine might achieve AAA-looking output or a Unity-like feature list much sooner than it achieves Unity-like trust and robustness.

The CAD project you saw may be technically remarkable while still being very different from replacing SolidWorks in an engineering company. That does not diminish it—it simply shows that implementation scale and product maturity are different dimensions.

The genuinely powerful compounding effect

You will have two improving systems:

The AI workforce becomes more capable.
The repository accumulates tools, tests, benchmark scenes and working subsystems.

Those can multiply each other.

A future agent entering a mature repository with:

deterministic test scenes;
visual reference images;
GPU performance histories;
architecture documentation;
clean module boundaries;
thousands of regression tests;
recorded failed approaches;
automatic builds and captures;

will be vastly more capable than the same agent dropped into an unstructured 500,000-line codebase.

The most valuable thing you may build this year is therefore not the code itself. Future people will be able to generate huge quantities of code too. Your moat becomes:

A coherent product vision.
A year of engineering decisions.
Visual taste and art direction.
Benchmark scenes and evaluation infrastructure.
Known failure cases.
A working game that exercises the engine.
Eventually, users and community knowledge.
Build the game and engine together

I wouldn’t spend a year building a theoretical general-purpose engine.

Make the herding game the engine’s permanent integration test:

Need a huge flock? Build animation and simulation LOD.
Need a beautiful hillside? Build terrain streaming and vegetation.
Need golden-hour fog? Build volumetrics and atmosphere.
Need deformable terrain? Build voxel editing and incremental updates.
Need stable third-person visuals? Build temporal tests around the actual camera.

That prevents AI from generating hundreds of impressive but unnecessary systems.

Once the herding game works, build one extremely small second project with the engine. If it can support another voxel game without tearing everything apart, you have begun turning game technology into an actual engine.

Design the repository for next year’s AI

Your instruction to Codex should effectively be:

We are building a multi-year, AI-maintained codebase. Optimize for future agent comprehension, mechanical verification, replaceable subsystems and architectural coherence—not merely rapid feature accumulation.

From the beginning, maintain:

A short architectural map.
Explicit module ownership and dependency rules.
Decision records explaining important choices.
One-command build, test and benchmark workflows.
Known-good screenshots and camera routes.
Performance budgets.
Regular removal of dead and duplicated code.
Feature flags and rollback points.
A strict rule that new features cannot silently break old ones.

Anthropic found exactly this in its large experiments: the limiting factor became the environment, tests, feedback and context handoff—not raw code generation. Its long-running application work now uses separate planner, implementation and evaluator roles to maintain progress and judge subjective quality. Anthropic’s long-running application research

So my real extrapolation is:

After one year, you probably won’t possess “another Unity.” But you could possess something that would have required a small, highly specialized engine team several years to build—a beautiful voxel game plus a genuinely reusable engine/editor underneath it.

And under an aggressive progress scenario, the project could stop being merely “the engine for my herding game” and become its own product: a focused AI-native platform for building procedural voxel worlds.

That is ambitious, but based on what agents are already doing in August 2026, it is no longer a ridiculous ambition.
