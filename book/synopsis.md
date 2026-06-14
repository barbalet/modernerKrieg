# Synopsis

`modernerKrieg` is a portable C engine for a Mosul demo, not a platform app
with rules scattered behind buttons. The intended Mac and Windows applications
are native wrappers over a stable C contract. The engine owns the battle:
scenario loading, map scale, multi-level topology, orders, movement, contact
reports, soldiers, civilians, combat consequences, scoring, replay, and AI-only
simulation.

That choice is inherited in spirit from `derZweiteWeltkrieg`, but the remit is
different. `derZweiteWeltkrieg` is a tabletop World War 2 rules engine with a
SwiftUI interface. `modernerKrieg` is an urban combat and civilian-risk engine
for a specific modern demo slice. It must carry more C-side detail because the
native frontends should not decide how hidden threats, civilians, routes, roof
access, or search outcomes behave.

![Market / Commercial Streets overview](../assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png)

The first public target is the 2003 Market / Commercial Streets battle slice:
a 500 m by 500 m urban security fight with U.S. patrol forces, irregular armed
threats, civilians, shop interiors, alley routes, roof access, caches, and
civilian-risk consequences. It is loaded from data, not just from a hard-coded
fixture. The scenario file is
[`../game/mosul/scenarios/market_commercial_streets_2003.mkscenario`](../game/mosul/scenarios/market_commercial_streets_2003.mkscenario).

## Reader Path

Chapter 1 explains the overall shape: pure C core, Mosul content, asset
validation, renderer-independent board views, native-wrapper API, command-line
tools, and tests.

Chapter 2 reads the public C boundary. `mk_core.h` is the engine contract, and
`mk_demo.h` is the contract native frontends should begin with.

Chapter 3 describes the scenario as data: controllers, factions, forces,
spawn zones, civilians, objectives, score weights, asset references, and
after-action text.

Chapter 4 covers the multi-level city: building-level manifests, topology
nodes, portals, semantic zones, vertical links, and how the map becomes
movement and line-of-sight terrain.

Chapter 5 follows combat units down into individual soldiers. This is where the
project differs from a simple unit-counter game: the unit remains the command
object, but soldiers carry stance, wounds, stress, ammo, role, and equipment.

Chapter 6 gives civilians their own place in the model. Civilians are not
ambient score tokens. They have state, stress, intent, routes, groups, and
exposure to danger.

Chapter 7 explains hidden information, suspected contacts, false contacts,
search events, cache discovery, and the need for the engine to keep imperfect
knowledge deterministic.

Chapter 8 describes movement, LOS, cover, and urban surfaces. This is the
mechanical middle of the project: world meters, pixels, nodes, portals, routes,
blockers, and elevations must agree.

Chapter 9 covers combat, risk, and restraint. The important question is not
only whether a shot hits. It is whether the simulation preserves suppression,
casualties, civilian risk, objective pressure, and future legality.

Chapter 10 covers AI-only battle runs, replay, CI, logs, and why the project
uses deterministic evidence rather than trust in one interactive playthrough.

Chapter 11 is the graphics chapter. It treats art as atmosphere, interface,
asset contract, and review surface. The current 1,088 rendered runtime sprites
and map plates are part of the design language.

Chapter 12 closes with extension discipline: how to add scenarios, assets,
rules, AI, or native UI without losing the C-level source of truth.

The [Art Plate Index](art_plate_index.md) is the review gallery for the book.
It gathers the map, level stack, line art, sprite sheets, and runtime examples
that the chapters use as visual anchors.

## Evidence Source Map

The most important source files are:

- [`../engine/core/include/mk_core.h`](../engine/core/include/mk_core.h), the
  public C simulation contract.
- [`../engine/core/src/mk_core.c`](../engine/core/src/mk_core.c), the current
  deterministic game implementation.
- [`../engine/demo/include/mk_demo.h`](../engine/demo/include/mk_demo.h), the
  native-wrapper session API.
- [`../engine/demo/src/mk_demo.c`](../engine/demo/src/mk_demo.c), the wrapper
  implementation that loads Mosul, steps AI, exports draw commands, and handles
  picking.
- [`../engine/assets/include/mk_asset_manifest.h`](../engine/assets/include/mk_asset_manifest.h),
  the asset manifest contract.
- [`../engine/assets/src/mk_asset_manifest.c`](../engine/assets/src/mk_asset_manifest.c),
  the C manifest parser and validator.
- [`../engine/render/include/mk_board_view.h`](../engine/render/include/mk_board_view.h),
  the renderer-independent board and overlay projection API.
- [`../engine/ai/src/mk_ai.c`](../engine/ai/src/mk_ai.c), the tactical AI policy
  layer.
- [`../game/mosul/src/mk_mosul_demo.c`](../game/mosul/src/mk_mosul_demo.c), the
  Mosul scenario-loading bridge.
- [`../game/mosul/scenarios/market_commercial_streets_2003.mkscenario`](../game/mosul/scenarios/market_commercial_streets_2003.mkscenario),
  the primary scenario data.
- [`../assets/mosul/manifests/market_commercial_streets_2003_building_levels.json`](../assets/mosul/manifests/market_commercial_streets_2003_building_levels.json),
  the multi-level building manifest.
- [`../assets/mosul/manifests/market_commercial_streets_2003_topology.json`](../assets/mosul/manifests/market_commercial_streets_2003_topology.json),
  the tactical topology manifest.
- [`../assets/shared/manifests/shared_tactical_sprites.spritemanifest`](../assets/shared/manifests/shared_tactical_sprites.spritemanifest),
  the compact sprite manifest used by C validation.
- [`../assets/shared/runtime/sprites/rendered/render_manifest.json`](../assets/shared/runtime/sprites/rendered/render_manifest.json),
  the full runtime render manifest.

## The Book's Argument

The explicit part of `modernerKrieg` is familiar engineering: headers,
structs, arrays, validation, tests, pathing, AI, replay, and draw commands.

The esoteric part is quieter. The engine is trying to model uncertainty, fear,
visibility, restraint, and the way a dense city changes what a soldier, a
civilian, and a commander can know. Those things cannot be left to an
atmospheric UI layer. If they matter to play, they must become C state,
repeatable events, and testable consequences.

The graphics are unusually important for that reason. The map is not a generic
background. The roof plates, walls, doors, shop rows, civilian silhouettes,
weapon sprites, and traffic sprites make the invisible C state legible. The
book uses them as documentation because the simulation needs them as evidence.

## Current Reading Modes

Read the book in one of three ways:

- As architecture: follow Chapters 1, 2, 8, 10, and 12.
- As Mosul content: follow Chapters 3, 4, 6, 7, 9, and the art index.
- As frontend preparation: follow Chapters 2, 8, 10, 11, and 12.

The best reading is still linear. The chapters deliberately move from engine
shape to content, then from systems to evidence and extension.
