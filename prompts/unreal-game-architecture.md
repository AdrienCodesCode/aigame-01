# Task: establish a proportional Unreal game foundation

## Inputs

- Unreal version and target platforms: [VERSIONS]
- C++/Blueprint ownership preference: [RULE]
- Networking requirements: [NONE OR DESCRIBE]
- Approved vertical slice: [PATH]

## Assignment

Audit modules, plugins, maps, GameMode/GameState ownership, Pawns, Controllers,
Enhanced Input, animation, UI, assets, saving, and automation. Define explicit
C++ and Blueprint boundaries: stable rules and reusable systems in testable C++,
authored composition and presentation in small Blueprints. Avoid base classes or
subsystems that accumulate unrelated game behavior.

Use Gameplay Ability System, Mass, World Partition, CommonUI, dedicated servers,
or marketplace plugins only when their costs match an approved requirement.
Separate collision primitives and authoritative rules from skeletal presentation.

## Deliverables

- Module/plugin dependency map and asset naming/location rules.
- One packaged smoke map proving lifecycle, input, pause, and clean travel.
- DataAsset/configuration, save-version, and replication ownership rules.
- Automation tests and one target-platform packaged smoke run.
- Architecture, build, extension, and troubleshooting documentation.

## Gate

The editor result is not enough. Report cook/package results, target hardware,
shader/build time, and every plugin or platform left unverified.
