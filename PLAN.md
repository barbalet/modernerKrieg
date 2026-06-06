# modernerKrieg Plan

`modernerKrieg` is the portable tactical engine and runtime for the public MOSUL demo. The engine should now be developed as a PNG-backed tactical loader and renderer with deterministic gameplay behind it: the player sees authored Mosul map art, sprites, and tactical markers, while the portable C core owns movement, line of sight, fire, suppression, casualties, civilian risk, scenario state, AI, and replayable outcomes.

The first public target is the 2003 Market / Commercial Streets demo. `derZweiteWeltkrieg` remains useful design memory for deterministic rules, tests, and thin presentation layers, but it is not a dependency, submodule, naming model, or era model for MOSUL.

## Current Baseline

- CMake project, portable C core, Mosul game module, renderer-independent board-view code, and headless tests exist.
- The core already supports deterministic game state, unit selection, orders, movement ticks, line of sight, cover checks, unit fire, ammo spend, wounds, casualties, suppression, and morale state changes.
- The render layer already projects map, terrain, objectives, units, soldier offsets, selection, and movement targets into screen space.
- The SDL3 app shell is optional and experimental. If SDL3 is available, it provides the current launchable app path; if not, the core and tests still build.
- Source art for the 2003 demo is imported under `assets/mosul/source/`.
- Public source art includes line-art references, 128 px top-down sprite sheets, source-angle weapon sprites, and Market / Commercial Streets map/layer assets.
- The current hard-coded code scenario is still a placeholder and must be redirected from the older East Mosul/Gogjali shape to the 2003 Market / Commercial Streets demo.

## Runtime Shape

The playable app should behave primarily as a PNG loader and renderer with gameplay state behind it.

- Load map art, tactical sprites, markers, and metadata from manifests.
- Render the map as image layers or generated tiles, not as colored terrain rectangles.
- Render units from sprite metadata, facing, role, side, stance, casualty state, and selection state.
- Render tactical overlays for orders, routes, line of sight, objectives, suppression, hidden contacts, civilian risk, breach/search points, and rooftop access.
- Keep SDL/SwiftUI/frontend code as presentation and input handling only.
- Keep rules, scenario state, combat, AI, and scoring in portable C.

This separation keeps the first app simple: art on screen, gameplay in the core, and a thin bridge between them.

## Demo Target

The first playable demo is a compact scenario, not the whole city.

- Era: 2003, after Mosul's fall during Operation Iraqi Freedom.
- Area: 500 m x 500 m Market / Commercial Streets cluster.
- Player force: U.S. Army patrol/security element with squad-level control.
- Opposing force: regime remnants, irregular fighters, weapons looters, early insurgent cells, and hidden armed threats.
- Civilians: protected non-combatants whose location and movement affect player choices.
- Terrain: streets, shopfronts, courtyards, rooftops, upper floors, checkpoints, rubble, blocked routes, cache/search zones, and breach points.

The demo is playable when the user can launch one scenario, inspect the PNG map, select units, issue orders, resolve contact, see suppression/casualties/civilian risk, and reach a clear after-action outcome.

## Repository Direction

```text
modernerKrieg/
  CMakeLists.txt
  PLAN.md
  README.md
  assets/
    mosul/
      source/       unmodified source art and provenance notes
      runtime/      generated runtime assets, safe to rebuild
      manifests/    map, sprite, marker, and scenario-art metadata
      maps/         playable map products and tile metadata
      atlases/      packed atlas images and metadata
  docs/
    asset_pipeline.md
    engine_architecture.md
    scenario_format.md
  engine/
    core/           portable rules and state
    render/         renderer-independent board projection
    platform/sdl3/  optional SDL3 app shell
    tools/          asset/scenario validation and generation
  game/
    mosul/
      data/
      scenarios/
      src/
  tests/
    core/
    render/
    mosul/
    assets/
```

`engine/core` must not depend on SDL, graphics APIs, operating-system UI, or Mosul-specific art files. The Mosul game module can depend on the engine. The app/frontend can depend on both.

## Asset Pipeline

Source assets stay unmodified. Runtime files should be generated, validated, and rebuildable.

Current source asset groups:

- `assets/mosul/source/line_art/`: Mosul context, combatant, weapon, vehicle, and urban tactics plates.
- `assets/mosul/source/sprite_sheets/`: 128 px combatant, stance, vehicle, and weapon source sheets.
- `assets/mosul/source/sprite_sheets/source_angles/weapons_128/`: approved source-angle weapon PNGs.
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/`: Market / Commercial Streets map previews, layer manifest, and source map layers.
- `assets/mosul/source/notes/`: provenance and demo asset selection notes.

Immediate asset work:

- Define a map manifest that records source image path, world size, pixels per meter, origin, layer kind, z order, alpha behavior, and collision/pathfinding output paths.
- Define a sprite manifest that records source sheet path, tile size, pivot, role, side, state, facing, scale, and runtime id.
- Define marker assets for selection, move routes, fire orders, overwatch, suppression, casualty, objective, hidden contact, breach/search, rooftop/stair access, and civilian risk.
- Generate runtime map products outside `source/`: first a single overview image, then tiles when the image size becomes too large for smooth rendering.
- Generate runtime sprite/atlas products outside `source/` and keep the source sheets untouched.

Additional art probably needed for the first demo:

- civilian/non-combatant top-down sprites
- order and status UI icons
- hidden-contact and suspected-danger markers
- breach, search/cache, checkpoint, rooftop, stair, and blocked-route markers
- collision/pathfinding masks for roads, interiors, rooftops, rubble, and upper-floor access

Do not block gameplay development on a large new art pass. Add missing assets only when the playable slice needs them.

## Scenario Data

Move Mosul content out of hard-coded C constants.

Start with a compact validated data format for:

- scenario metadata and briefing
- factions and sides
- weapons and ammunition
- soldier templates
- unit templates
- initial unit placement and hidden state
- terrain/navigation regions
- objective definitions
- civilian-risk rules
- opposing AI plan
- scoring and after-action text

The C core should load validated runtime structs. Tests should reject invalid ids, invalid map bounds, missing assets, invalid faction references, impossible objectives, and unsupported weapon/unit combinations.

## Gameplay Systems

The demo should prove the modern urban problem before broadening scope.

- Orders: move, hold, fire, suppress, overwatch, breach/search, rally, withdraw.
- Combat: line of sight, cover, small arms, machine-gun suppression, RPG/direct threat, wounds, casualties, ammo, and stress.
- Urban movement: road, alley, interior, rooftop, stair, rubble, blocked route, and breach/search interactions.
- Civilians: presence, risk, harm scoring, movement constraints, and consequences.
- Uncertainty: hidden contacts, suspected threats, reveal logic, and false certainty.
- AI: defend, ambush, displace, flee, and protect hidden positions.
- Outcome: objective state, civilian harm, force preservation, time/turn pressure, and after-action summary.

## Frontend Choice

SDL3 remains the fastest way to validate a cross-platform runtime, but it is still a choice, not an identity.

- Keep building and testing the SDL3 app if SDL3 is available.
- Keep the C core portable enough for a SwiftUI frontend if SDL3 does not produce the desired feel.
- Do not put rules, scenario decisions, or asset interpretation exclusively in frontend code.

## Milestones

### Milestone A: PNG Map On Screen

- Add map asset metadata.
- Load a Market / Commercial Streets PNG map source or generated runtime image.
- Render it through the current board view with pan/zoom.
- Preserve map-to-screen and screen-to-map picking.
- Keep headless tests passing.

### Milestone B: Real Unit Sprites

- Add sprite metadata for the first U.S. patrol and one opposing cell.
- Render unit sprites from role/side/state instead of colored rectangles.
- Render selection rings, movement targets, and order lines over sprites.
- Add a fallback marker for missing assets.

### Milestone C: 2003 Scenario Data

- Create the first Market / Commercial Streets scenario data file.
- Replace or deprecate the hard-coded East Mosul placeholder.
- Load factions, units, objectives, and map metadata from data.
- Add parser/validation tests.

### Milestone D: Playable Contact

- Add visible fire/suppression/casualty feedback.
- Add hidden-contact reveal logic.
- Add civilian-risk scoring and at least one non-combatant constraint.
- Add a basic opposing AI response.

### Milestone E: Demo Polish

- Add briefing and after-action summary.
- Add deterministic replay/debug logging.
- Package one macOS-first smoke-tested build.
- Document how to build, run, test, and regenerate runtime assets.

## Immediate Next Steps

1. Add `docs/asset_pipeline.md` describing source assets, runtime assets, manifests, scale, pivots, facings, and generated outputs.
2. Add the first map manifest for Market / Commercial Streets.
3. Render the Market / Commercial Streets PNG in the app through the existing board-view transform.
4. Add the first sprite manifest and render at least one U.S. unit and one opposing unit as PNG sprites.
5. Rename or replace the hard-coded `mk_mosul_make_east_block_scenario` path with a 2003 Market / Commercial Streets scenario path.
6. Add tests for manifest validation and 2003 scenario loading.
7. Add visible order, selection, suppression, casualty, objective, and hidden-contact markers.

## Quality Bar

- Every new rule gets a deterministic test.
- Every manifest or scenario file gets validation.
- Missing art should produce a readable fallback, not a crash.
- Source assets remain unmodified.
- Runtime assets are reproducible.
- The demo must be playable at unit scale while preserving meaningful soldier-level consequences.
