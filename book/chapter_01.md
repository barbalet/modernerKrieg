# Chapter 1: The Shape Of The Demo Engine

The first thing to understand about `modernerKrieg` is that the game is not
arranged around a windowing library. The previous SDL experiment has been
removed, and the current repository points toward native Mac and Windows
frontends above a portable C engine. That is not only a platform preference. It
is the design center of the codebase.

The engine must be able to run without a screen. It must load Mosul data, step
the battle, run both tactical sides under AI, move civilians, produce replay
events, validate assets, export draw commands, and report failures in CI. A
native frontend can be beautiful, but it should never be the only place where a
gameplay truth exists.

![Mosul area line art](../assets/mosul/source/line_art/01_mosul_area_map.png)

## The Current Layers

The repository is divided into a small number of layers:

- [`../engine/core/`](../engine/core/) owns deterministic simulation state.
- [`../engine/assets/`](../engine/assets/) owns manifest parsing and validation.
- [`../engine/render/`](../engine/render/) owns renderer-independent board
  projection and overlays.
- [`../engine/ai/`](../engine/ai/) owns policies that emit ordinary orders.
- [`../engine/demo/`](../engine/demo/) owns the C session API intended for
  native wrappers.
- [`../engine/tools/autoplay/`](../engine/tools/autoplay/) owns headless runs,
  replay validation, and AI-vs-AI battle tooling.
- [`../game/mosul/`](../game/mosul/) owns Mosul-specific scenario content.
- [`../assets/mosul/`](../assets/mosul/) owns source art, manifests, runtime
  images, sprite sheets, and generated/cached runtime products.
- [`../tests/`](../tests/) owns deterministic checks.

This shape matters because modern urban combat needs many systems to agree.
The map says where streets, doors, windows, rooms, roofs, and rubble exist. The
core says whether a unit can move, see, fire, search, or breach. The AI says
what an actor intends. The renderer says how the result is drawn. If any one of
those layers starts inventing private rules, the demo stops being explainable.

## What C Owns

The core contract begins in
[`../engine/core/include/mk_core.h`](../engine/core/include/mk_core.h). It
defines sides, controllers, units, soldiers, orders, terrain, civilians,
traffic vehicles, contacts, search outcomes, breach outcomes, scoring, map
geometry, gameplay area records, topology records, snapshots, and after-action
reports.

The implementation in [`../engine/core/src/mk_core.c`](../engine/core/src/mk_core.c)
keeps the simulation deterministic. It stores bounded arrays for units,
civilians, objectives, terrain zones, contact reports, map tiles, topology
nodes, portals, semantic zones, and route state. That bounded C style is not an
accident. It makes the engine portable and inspectable. It also makes failures
simple: invalid argument, capacity, not found, or invalid data.

This is not a small concern. The demo aims to capture individual soldiers
inside commandable units. A unit can be the object the player orders, while its
members still carry role, stance, wound state, ammo, stress, exposure, and
cooldown information. The state must remain visible to tests, AI, replay, and
native wrappers.

## What Mosul Owns

The reusable engine should not know that every future battle is Mosul. The
Mosul layer adapts the specific content into engine data:

- scenario metadata and score thresholds;
- controller, faction, and force records;
- map size, tile hints, terrain zones, and objectives;
- spawn zones, unit templates, civilian archetypes, and civilian groups;
- manifest paths for maps, sprites, markers, building levels, and topology.

That content currently lives under [`../game/mosul/`](../game/mosul/), with the
primary scenario in
[`../game/mosul/scenarios/market_commercial_streets_2003.mkscenario`](../game/mosul/scenarios/market_commercial_streets_2003.mkscenario).
The loader path connects scenario data to the core without making the core a
Mosul-only library.

The same separation should hold as the project grows. Mosul-specific text,
asset references, and scenario tuning can stay above the core. Movement, line
of sight, combat, civilian state, and replay should stay in the core.

## What The Native Frontends Own

The native frontends should own:

- windows, menus, panels, platform input, and accessibility;
- image loading and GPU upload;
- Metal or platform-specific rendering decisions on Mac;
- Windows presentation technology later;
- native save/open panels and packaging;
- ergonomic UI for selecting units, reading reports, and viewing overlays.

They should not own:

- whether a shot is legal;
- whether a civilian panics;
- whether a door blocks movement;
- whether a suspected contact is false;
- whether a unit has reached a topology route waypoint;
- whether a search finds a cache;
- whether a replay remains valid.

The native-wrapper API in
[`../engine/demo/include/mk_demo.h`](../engine/demo/include/mk_demo.h) is the
practical boundary. A platform app can create a session, load the default
Mosul scenario, step fixed ticks, query snapshots and draw commands, pick from
screen coordinates, select a unit, issue a selected move, get after-action
text, and inspect scenario/topology debug text.

## Why The Art Belongs In The Architecture

In many games, art can be documented after the rules. Here the art is closer to
the rules than that. The map manifests define world size and pixels per meter.
The building-level manifest defines the vertical image stack and first physical
features. The topology manifest names rooms, portals, roofs, caches, shelters,
and danger zones. The sprite manifests bind roles and states to runtime images.

The current runtime map overview is a 1,400 px review image. The gameplay
level stack uses 7,000 px PNGs at 14 px/m. Those numbers are not aesthetic
alone. They connect line art to a 500 m by 500 m world. If a frontend draws a
soldier at the wrong scale, the player sees it. If a route follows a node that
does not match the floor plate, the player feels it. If a roof is visible but
not pathable, the simulation becomes false in a way that is hard to unsee.

## The Headless Promise

The command-line tools make the engine accountable:

- `mk_headless_run` loads and steps scenarios.
- `mk_ai_battle` runs deterministic AI-vs-AI battle sweeps.
- `mk_replay_validate` checks replay files and can play back event timelines.

Those tools are not temporary scaffolding. They are how the project proves that
native wrappers are not hiding simulation defects. A Mac app can show the
battle beautifully, but a headless AI battle should still reveal bad routes,
stalled orders, broken civilians, score drift, missing assets, or replay
incompatibility.

## The Design Promise

The rest of this book treats `modernerKrieg` as a layered promise:

- C owns battle truth.
- Mosul content feeds that truth with data.
- The asset layer validates what the renderer and simulation will rely on.
- The board-view layer projects state without becoming a renderer.
- AI emits ordinary orders instead of private actions.
- Native wrappers display and command the engine.
- CI and replay keep the engine honest.

That promise is what lets the project become more detailed without becoming
fragile. The city can gain interiors, traffic, more civilians, richer hidden
contacts, and platform-native presentation because the core already knows where
truth belongs.

## Failure Modes This Shape Prevents

The layer split is not ceremony. It prevents specific failures:

- a Mac-only rule that cannot run in CI;
- a Windows rule that disagrees with Mac play;
- a replay that validates only when a renderer is present;
- an AI run that cannot see civilian state because civilians were UI objects;
- a map that looks enterable but has no topology id;
- a sprite id used by a scenario but absent from the manifest;
- a route that succeeds visually but fails in headless simulation.

When a future change feels convenient but crosses a layer boundary, ask whether
it reintroduces one of those failures. If it does, the convenience is probably
too expensive.
