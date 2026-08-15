Edit
Edit

With a custom voxel engine, I’d specifically chase loops where the world itself is the simulation—not just “normal game, but blocky.” That’s where building your own engine becomes creatively justified.

Here are some directions that could feel genuinely distinctive:

Living avalanche shepherd — You don’t herd animals; you herd snow. You’re a mountain worker trying to deliberately trigger, split, redirect and dissipate avalanches before they destroy villages. Every voxel has snow mass, cohesion, temperature and slope stability. You cut trenches, blast cornices, build deflection walls and deliberately create smaller avalanches. The spectacle is thousands/millions of voxels behaving like granular matter.
Underground ecosystem engineer — You control almost nothing directly. You alter water, heat, nutrients and tunnels, then watch roots, fungi, insects and underground creatures colonize the voxel world. Your objective might be to restore a dead landscape from underneath. Digging a channel three meters differently could eventually change an entire ecosystem.
The moving city — A gigantic voxel city is slowly migrating across an infinite hostile landscape. Buildings can literally be dismantled voxel-by-voxel and reconstructed further ahead. Your loop is deciding what parts of civilization move, what gets abandoned, and how to cross rivers/mountains. The whole settlement becomes a kind of enormous organism.
Architectural horror where geometry is the enemy — You're maintaining an apartment building whose internal topology is slowly becoming impossible. Rooms expand, walls migrate, shafts connect to the wrong floors, staircases grow downward. Because it's voxel-native, the transformations are actual geometry, not scripted teleports. Your job is to keep the building habitable while investigating why it is changing.
Firefighter / wildfire simulation — A forest made entirely from combustible voxel materials where fire propagates according to fuel, moisture, wind and heat. You cut firebreaks, redirect rivers, fell trees, back-burn areas and evacuate settlements. A huge selling point would be that every intervention physically changes how the fire spreads.
Creature made of terrain — The entire map is one colossal sleeping creature. Mountains are its spine, caves are organs, rivers are blood vessels. You're tiny. Mining, building and extracting resources changes the creature's physiology, eventually waking it. Gameplay becomes learning that what looked like geology is anatomy.
Voxel archaeology without combat — You excavate buried settlements layer by layer. Every voxel you remove destroys context, so excavation itself is the core puzzle. Soil strata, collapsed buildings, bones, artifacts, ancient fires and waterways let you reconstruct what happened. Procedural generation could create internally consistent historical sites rather than random loot caves.
Flood engineer — An entire game around water finding its own path through terrain. A settlement sits in a giant valley and increasingly violent storms arrive. You're not building arbitrary tower-defense walls; you're reshaping watersheds, carving spillways, restoring wetlands, opening old river channels and sacrificing areas intentionally. Every rainstorm becomes a giant physics experiment.
The world is slowly crystallizing — An alien crystalline growth spreads through matter voxel-by-voxel. It converts terrain according to pressure, material and nearby crystal orientation. You manipulate temperature, vibration and geometry to guide its growth. Instead of “kill the infestation,” the central mechanic is growing structures through constrained emergent rules.
Human ant colony — You're managing a settlement entirely underground, except there's no omniscient city-builder interface. You physically inhabit one person and issue high-level instructions. Excavated volume, airflow, heat, smoke, water drainage and structural collapse are simulated. Over time, the settlement becomes an enormous emergent 3D machine carved into the earth.

A few of these become particularly interesting because a voxel engine gives you something Unity-style games often fake:

material can actually move.

That's potentially your killer feature.

Instead of thinking:

“What games look good in voxels?”

I'd think:

“What gameplay becomes possible when the world consists of discrete material that can be removed, deposited, transported, burned, melted, flooded, collapsed or transformed?”

That leads to much stranger loops.

For example, one concept I particularly like:

You herd a river

No character combat, almost no conventional objectives.

You're some kind of ancient hydraulic keeper living in a mountainous voxel world. Water comes down from glaciers every spring. Villages, forests and farmland depend on it.

You physically manipulate the terrain:

dig a channel → water follows it
collapse a hillside → river dams
cut through rock → create a waterfall
open a basin → create a lake
divert water → one village thrives while another dries out
too much velocity → erosion destroys your channel
sediment deposits → the river slowly changes itself

So your “character” is effectively controlling a gigantic evolving organism made from water.

The progression wouldn't be better pickaxes. It could be increasingly complicated watersheds.

Early:

spring → field

Later:

          reservoir
         /         \

glacier → river → town
\ \
 wetlands mills
\
 farmland

And then seasons modify everything.

That one has almost the same appeal as your sheep idea: you don't directly control the thing you're controlling. You influence it spatially and watch emergent behavior propagate through the system.

That may actually be a fertile design principle for the whole engine:

indirect-control games.

Sheep.
Water.
Fire.
Snow.
Sand.
Crowds.
Roots.
Fungus.
Smoke.
Insects.
Lava.
Disease.
Light.

The player manipulates the environment and the simulated mass responds.

That's considerably more original than using the engine to make another voxel survival game.

Ah ok so voxels are mostly known for being able to modify environments?

Exactly — that’s one of the big reasons voxels became associated with destructible/modifiable worlds, but it’s not the only reason to use them.

A voxel world is essentially a 3D grid of little cells. So instead of your terrain being one continuous triangle mesh, the world can be represented roughly like:

[x,y,z] = air
[x,y,z] = dirt
[x,y,z] = stone
[x,y,z] = water
...

That makes operations like:

remove this chunk of rock
put dirt here
burn these cells
flood this cavity
collapse this wall
dig a tunnel

conceptually very natural. You are literally changing the world's data.

That’s why Minecraft is the obvious example, and why voxel tech often appears in games involving mining, destruction or procedural terrain.

But voxels have several other interesting properties.

True 3D interiors are easy to represent. A mountain isn't merely a surface—you can have rock, caves, tunnels, ore, water and empty space throughout its volume.

Procedural generation maps nicely onto them. Mathematical noise/functions can directly say what material occupies each coordinate. For the game you're having Codex make, you can essentially generate the whole countryside mathematically.

Simulation can interact with the same grid. Imagine every voxel storing not just material:

material
moisture
temperature
snow depth
flammability
light
water content

Suddenly your terrain is also your simulation.

And that's where custom voxel engines can become really interesting.

For example:

FOREST VOXEL

wood
temperature = 20°
moisture = 0.3
burning = false

↓ nearby fire

temperature = 350°
burning = true

↓ later

material = ash

The geometry changes naturally because the underlying world state changed.

But voxels absolutely don't need to be editable

Your sheep game could have a completely static voxel landscape.

There, voxels mainly give you:

a distinctive visual language + cheap procedural geometry + potentially enormous landscapes + custom lighting opportunities.

The sheep themselves don't even technically need to be voxels. You could render conventional meshes inside a voxel-generated world.

There's also a slight terminology trap here.

The images we generated are really “voxel-art-looking.” That's an aesthetic.

A true voxel engine is a world representation/rendering architecture.

You can have:

voxel-looking graphics with ordinary meshes
a true voxel engine that visually looks almost realistic
a true voxel world that is completely immutable
a voxel world where literally everything can be destroyed

So when Codex is building yours in C++/OpenGL, the interesting question is not merely “does it look voxel?”

It's:

What information does each voxel actually contain, and what can the player/simulation do to it?

If every voxel is just grass/rock/air, you're mostly getting terrain and aesthetics.

If voxels can carry state and change over time, then you've suddenly got a simulation platform, and that's where a custom engine can enable some genuinely unusual game loops.

Today 8:52 AM
6 months to code everything with codex ? Nah bro..
U underestimate your own capabilities lol
Checkout the reddit post about lumenfall
That guy build the engine in 1 week, plus an environment

Bro.

😂 Yeah. Bro, point taken. My six-month line was badly calibrated for 2026 agentic coding.

I found the LUMENFALL post you meant, and it’s almost hilariously close to what you're asking Codex to do.

The author says Claude Opus + Fable wrote essentially all the code while he acted more as director/QA: identifying visual problems, requesting systems, playtesting, rejecting bad results, and iterating.

And this wasn't just draw some cubes:

C++ + raw OpenGL
custom voxel engine, no Unity/Unreal
no external libraries
procedural terrain, biomes, caves, castles, villages and creatures
ray-marched volumetric clouds
cascaded shadow maps + PCSS shadows
water reflection/refraction
TAA
SSAO
god rays
volumetric sky + day/night
multithreaded chunk generation/meshing
LOD/chunk streaming
roughly 450 chunks visible
tiny executable because the content is procedurally generated.

And yes:

the author repeatedly says it took one week.

The particularly relevant bit for your experiment is that it wasn't a one-shot prompt. He says he initially requested an optimized C++ voxel engine targeting ~250 chunks, supplied FPS/1%-low performance goals, had the AI stress-test it repeatedly, increased the target as optimizations landed, and spent days debugging/tweaking things like shadows.

That's basically exactly the workflow you're proposing:

You specify architecture/behavior/performance → Codex implements → run → inspect → tell it what's wrong → Codex profiles/fixes → repeat.

So I would radically revise what I said earlier.

For your Border Collie project, I would no longer think:

engine = months
game = months afterward

I'd think more like:

First impressive vertical slice: potentially days–2 weeks

Not a shipping game, but something where you can already:

walk around → voxel terrain → volumetric lighting → hundreds of sheep → flocking → Border Collie pressure → shepherd commands → procedural environment.

And LUMENFALL demonstrates that even the rendering half of that statement isn't particularly insane anymore.

Then the time starts going into the interesting stuff, not “how do I open a window in OpenGL”:

Sheep simulation
↓
Dog-pressure model
↓
Realistic flock splitting/rejoining
↓
Terrain-aware movement
↓
Dog command system
↓
Objectives
↓
Procedural landscape generation
↓
Optimization
↓
Polish / feel

And actually, your game may be simpler than LUMENFALL in several rendering/content respects because you don't need Minecraft-style caves, castles, villages, complicated water, etc.

Instead you can tell Codex:

This engine exists to render huge outdoor landscapes beautifully and simulate enormous animal flocks efficiently.

That's a fantastic constraint for a custom engine.

You could deliberately omit 90% of what a general engine contains:

no skeletal animation system initially
no editor
no scripting language
no generalized physics engine
no scene graph designed for arbitrary games
no asset importer
no material editor
no NavMesh system
no prefab system
no animation state-machine editor

Just build the systems the game actually needs.

And here's where I think I was importing the old gamedev mentality incorrectly:

Historically, “build your own engine” implied:

you personally have to implement and debug thousands of pieces of infrastructure.

With Codex/Fable/Opus-class agents, the human bottleneck shifts toward:

knowing what architecture you want, defining constraints, evaluating the output, and steering iteration.

That's exactly what the LUMENFALL creator says happened.

So yeah.

Six months just to get the engine/game foundation? Nah. I withdraw that one. 😂

The much more interesting experiment now is seeing how far Codex gets on your voxel herding game in the first week. Given LUMENFALL, I would actually set the bar pretty damn high.
