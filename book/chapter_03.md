# Chapter 3: Mosul As Data

The Mosul demo is not only a fixture compiled into C. Its primary shape lives
in scenario and manifest data. The scenario file
[`../game/mosul/scenarios/market_commercial_streets_2003.mkscenario`](../game/mosul/scenarios/market_commercial_streets_2003.mkscenario)
names the battle, sets the seed, defines scoring, links assets, describes map
scale, declares controllers and forces, places terrain, establishes spawn
zones, and feeds population records into the core.

This matters because a data-backed scenario can be inspected, validated, and
varied. The same engine can later load smoke scenarios, balance scenarios,
debug scenarios, and the public demo without needing a new C build for every
experiment.

## Scenario Identity

The first lines are ordinary but important:

```text
format=modernerKrieg.scenario.v1
id=market_commercial_streets_2003
name=Market Commercial Streets 2003
seed=84985359904819
briefing=Secure the market junction, identify armed threats, and keep protected civilians out of the line of fire.
```

The seed is part of the scenario identity. AI-only battles, replay validation,
civilian movement, search outcomes, and combat results need deterministic
randomness. If a scenario cannot be repeated, it cannot be debugged well.

The briefing is also data. It flows through snapshots and headless output. It
is not a UI-only string because the scenario's purpose affects tests, audits,
and after-action interpretation.

## Scoring As Scenario Pressure

The scenario defines success and partial thresholds, objective weight, civilian
risk weight, casualty weights, and time weight. That turns the battle into a
pressure field. Securing the objective matters, but not at any cost. Delay
matters. Civilian risk matters. Player casualties matter. Civilian casualties
matter more.

The current score model is not a final theory of urban combat. It is a
starting contract. It tells AI and tests what the demo values. It also creates
a place where later nuance can enter: proportional risk, protected locations,
warnings, evacuation success, medical response, search intelligence, or
collateral damage.

## Asset References

The scenario links the key manifests:

- map manifest;
- sprite manifest;
- building-level manifest;
- topology manifest.

Those references are validated before the core accepts the scenario. This is
the correct direction. Scenario data should not be allowed to point at missing
images, missing topology, invalid sprite ids, or geometry with impossible
portal links.

![Runtime map overview](../assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png)

The current map is a 500 m by 500 m area with a 10 by 10 coarse tile grid. The
coarse grid remains useful for fallback terrain and broad tests, but the
building-level and topology manifests carry the future of the demo. The
long-term engine should ask "which node, portal, level, feature, and semantic
zone?" more often than it asks "which square?"

## Controllers, Factions, And Forces

The scenario currently names three controllers:

- U.S. patrol tactical AI;
- armed threat tactical AI;
- civilian observer.

It also names three factions:

- U.S. Army Patrol;
- Armed Irregular Cell;
- Civilians.

And three forces:

- U.S. Patrol Force;
- Armed Irregular Cell;
- Civilian Population.

Those are separate records for a reason. A faction is identity and color. A
force is the command element in this scenario. A controller is who makes
decisions. This allows the same content to be played human-vs-AI, AI-vs-AI,
scripted, observed, or eventually replay-driven.

## Spawn Zones

Spawn zones are one of the scenario's most useful data forms. They connect side,
level, bounds, capacity, and optional topology node id. A zone can be a patrol
entry, civilian crowd, civilian shelter, rooftop threat, cache guard, or hidden
scout.

This is better than placing every actor at a naked coordinate. A spawn zone
says why a placement exists. It gives the AI and scenario auditor a semantic
handle. It also gives future tools a way to randomize or vary within authored
limits.

The most important habit is to keep spawn zones tied to topology ids whenever
possible. A civilian in a shop row should know which shop node it belongs to.
An opfor rooftop threat should know its roof access node. A patrol entry should
know the street entry or crossing it can move from.

## Unit Templates And Soldier Detail

The scenario defines unit templates instead of only individual unit instances.
Templates describe role, side, training, default spawn zone, and expected
soldier count. The core can then instantiate units with soldier-level details.

This is the right compromise for the demo. The player and AI should command
units. But the battle should still feel like a patrol made of people with
different roles, weapons, wounds, and stress. A template lets content authoring
stay readable while the engine remains detailed.

## Civilian Archetypes And Groups

Civilian data should be treated with the same seriousness as combat data.
Archetypes connect behavior and sprites. Groups connect civilians to places,
routes, shelters, and social context. Mosul's density comes from civilians
being active agents, not just passive penalty markers.

The sprite side is already present. The runtime render manifest has 168
civilian sprites across archetypes, states, and facings. The scenario loader
validates civilian archetype sprite ids against the compact sprite manifest.

![Civilian top-down source sheet](../assets/shared/source/sprite_sheets/19_civilian_states_topdown_128.png)

The next level of richness should come from group intent: market crowd,
shopkeeper, family, wounded person, sheltering residents, evacuation stream, or
separated group. Each of those should be visible in replay and snapshots.

## Smoke Scenarios

The `game/mosul/scenarios/` directory contains additional smoke scenarios:

- blocked path;
- cache search;
- civilian panic;
- contested risk;
- control;
- empty;
- evacuation;
- interior contact;
- rooftop threat.

These are not secondary fluff. They are how the engine tests one idea at a
time. A full demo battle is too dense to debug every behavior at once. Smoke
scenarios should stay small, named, and deterministic.

## What Good Scenario Data Feels Like

Good scenario data should be boring to parse and interesting to simulate. It
should name the things a designer cares about without smuggling in code. It
should be strict enough that typos fail early. It should use stable ids because
replay, debug text, CI logs, and native UI picks need to talk about the same
places and actors.

The Mosul scenario is already headed in that direction. The next refinements
should deepen the topology and population data rather than returning to
hard-coded special cases.

## Validation Questions

Every scenario change should answer these questions before it is considered
ready:

- do all manifest paths exist?
- do all sprite runtime ids resolve?
- do all topology node ids exist?
- do all portal references point to valid nodes?
- are civilian groups tied to plausible spawn zones?
- can AI-only runs reach an outcome?
- does the after-action text still describe the score model?
- does a failed load produce a useful error?

These are not only test questions. They are authoring questions. Good content
should make broken assumptions visible early.

## Data Smells

The scenario is probably drifting in the wrong direction if:

- coordinates appear where topology ids would be clearer;
- a smoke scenario needs large unrelated data to test one behavior;
- score thresholds are changed only to quiet failing AI;
- a civilian group has no shelter, exit, or behavior reason;
- opfor placement relies on the AI seeing hidden state;
- a map change requires C constants to be patched by hand.

Those smells do not always mean the change is wrong. They do mean the change
needs extra review.
