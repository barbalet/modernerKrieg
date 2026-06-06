# modernerKrieg Plan

`modernerKrieg` is the portable tactical engine and runtime for the public MOSUL demo. The engine should now be developed as a PNG-backed tactical loader and renderer with deterministic gameplay behind it: the player sees authored Mosul map art, sprites, and tactical markers, while the portable C core owns movement, line of sight, fire, suppression, casualties, civilian risk, scenario state, AI, and replayable outcomes.

The first public target is the 2003 Market / Commercial Streets demo. `derZweiteWeltkrieg` remains useful design memory for deterministic rules, tests, and thin presentation layers, but it is not a dependency, submodule, naming model, or era model for MOSUL.

## Current Baseline

- CMake project, portable C core, Mosul game module, renderer-independent board-view code, and headless tests exist.
- The core already supports deterministic game state, unit selection, orders, movement ticks, line of sight, cover checks, unit fire, ammo spend, wounds, casualties, suppression, morale state changes, hidden-contact reveals, suspected/false contact reports, civilian-risk updates, objective control, scoring thresholds, and after-action summaries.
- The core state model now includes controller slots, force records, command identities, map tiles with elevation/cover/movement costs, richer weapon/ammunition metadata, richer soldier state, standalone civilian records, contact reports, hidden unit fields, and snapshot/load support for those containers.
- The render layer already projects map, terrain, objectives, units, soldier offsets, selection, movement targets, fire lanes, suppression, hidden contacts, casualties, and civilian-risk overlays into screen space.
- A deterministic headless runner exists for smoke tests and AI-vs-AI/autoplay work.
- CMake presets exist for default, headless, and strict warning-as-error builds.
- Asset pipeline documentation, the first Market / Commercial Streets map manifest, the first 2003 sprite manifest, the first marker manifest, and C manifest validation tests exist.
- The 2003 Market / Commercial Streets scenario now loads from a validated `.mkscenario` data file, with a C fixture retained for parity tests.
- `mk_headless_run` can load an explicit scenario path, override the seed, run for a fixed tick budget, suppress console output, write a transcript with contact/risk counters, briefing, debug/replay lines, balance expectations, and after-action output, and run both non-civilian tactical sides under basic AI.
- The SDL app can read the map, sprite, and marker manifests and, when SDL3_image is available, load the manifest PNG assets; otherwise it keeps fallback map/unit/overlay rendering.
- The first runtime map overview exists at `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`.
- The SDL3 app shell is optional and experimental. If SDL3 is available, it provides the current launchable app path; if not, the core and tests still build.
- Source art for the 2003 demo is imported under `assets/mosul/source/`.
- Public source art includes line-art references, 128 px top-down sprite sheets, source-angle weapon sprites, and Market / Commercial Streets map/layer assets.
- The remaining public-demo work is now presentation, marker, contact, AI, scoring, and packaging focused.

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
    build_matrix.md
    engine_architecture.md
    milestone_0_review.md
    milestone_1_progress.md
    scenario_format.md
    third_party.md
  engine/
    ai/             controller policies that emit core orders
    core/           portable rules and state
    render/         renderer-independent board projection
    platform/sdl3/  optional SDL3 app shell
    tools/
      autoplay/     headless runs, future AI-vs-AI batches
      assets/       asset/scenario validation and generation
  game/
    mosul/
      data/
      scenarios/
      src/
  tests/
    autoplay/
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
- controller assignments
- forces and command identities
- weapons and ammunition
- soldier templates
- unit templates
- initial unit placement and hidden state
- civilian placement and civilian state
- terrain/navigation regions
- map image/layer manifests
- sprite/marker manifests
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

## Completed Foundation

The following baseline work is complete and should be preserved while the plan pivots to PNG-backed public-demo work:

- Milestone 0 skeleton: CMake, optional SDL3 app shell, headless runner, fixed-step loop, test helper layer, build presets, CI notes, asset folder stubs, core-no-SDL smoke test, and architecture/third-party docs.
- Milestone 1 core-state first half: stable containers for controllers, forces, command identities, terrain tiles, civilians, units, soldiers, weapons, objectives, snapshots, scenario loading, and validation.
- Asset/data foundation pass: source-safe asset pipeline docs, map manifest, sprite manifest, manifest parser/validator, manifest CTests, SDL manifest handoff, and 2003 scenario entry point.
- Current tests cover deterministic RNG, scenario loading, scenario data validation, snapshots, movement, LOS, firing, suppression, board-view transforms, fixed-step runs, asset manifests, and the core/SDL boundary.

## Development Cycle Ledger

Cycle accounting starts from the current checked-in PNG-backed Mosul demo plan state, after the completed foundation above. Earlier foundation work is preserved as baseline work and is not renumbered into this ledger.

- Ledger baseline date: 2026-06-06.
- Planned cycle budget: 100 cycles.
- Planned cycle shape: 5 milestones x 20 cycles each.
- Completed cycles in this ledger: 50.
- Current cycle: 50.
- Remaining planned cycles: 50.
- Next cycle batch: cycles 51-60.

When a development batch is completed, increment `Completed cycles in this ledger`, advance `Current cycle`, reduce `Remaining planned cycles`, and update `Next cycle batch`. Add one log row describing the cycle range, milestone focus, completed work, and verification.

| Date | Cycles Completed | Current Cycle | Remaining Cycles | Milestone Focus | Notes |
| --- | ---: | ---: | ---: | --- | --- |
| 2026-06-06 | 0 | 0 | 100 | Baseline | Ledger added after the completed foundation pass; next work starts at cycles 1-10. |
| 2026-06-06 | 10 | 10 | 90 | Milestone C / Autoplay | Added the 2003 `.mkscenario` file, scenario parser, fixture parity and invalid-reference tests, default data-backed Mosul loader, and headless scenario/seed/tick/quiet/transcript controls. Verified default CTest and direct transcript run. |
| 2026-06-06 | 20 | 20 | 80 | Milestone B / Autoplay | Added marker metadata and validation, renderer-independent tactical overlays, SDL sprite/marker fallback rendering, copied the first runtime map overview, introduced basic AI order emission, and added AI-only headless smoke coverage. Verified default CTest and direct AI transcript run. |
| 2026-06-06 | 30 | 30 | 70 | Milestone D / Contact | Added contact reports for fire/reveal/civilian risk, hidden-contact scenario fields and reveal checks, civilian-risk updates from fire and proximity, fire/hidden overlays, smarter AI suppress/withdraw choices, and deterministic AI transcript assertions. Verified default CTest and direct AI transcript run. |
| 2026-06-06 | 40 | 40 | 60 | Milestone D/E / Outcome | Added persistent objective-control state, deterministic score math for objectives/civilian risk/casualties/time, after-action summary data, `mk_headless_run --aar`, player AI civilian-risk restraint, revealed-opfor withdrawal behavior, and deterministic tests for scoring/AAR/AI caution. Verified default CTest and direct AI-only AAR transcript run. |
| 2026-06-06 | 50 | 50 | 50 | Milestone D/E / Uncertainty + Replay | Added suspected-danger and false-contact records with confidence/terrain metadata, objective-control/suspected/false overlays, scenario briefing/AAR text and score thresholds, headless `--briefing`, `--debug-log`, `--expect-objective`, and `--expect-min-score`, plus CTest coverage for the new debug/balance path. Verified default CTest and direct briefing/debug/AAR transcript run. |

## Milestones

### Milestone A: PNG Map On Screen

- Complete: add map asset metadata.
- Complete: load a Market / Commercial Streets PNG map source or generated runtime image.
- Complete: render it through the current board view with pan/zoom when SDL3/SDL3_image is available, with fallback rendering otherwise.
- Complete: preserve map-to-screen and screen-to-map picking.
- Keep headless tests passing.

### Milestone B: Real Unit Sprites

- Complete: add sprite metadata for the first U.S. patrol and one opposing cell.
- Complete: render unit sprites from role/side/state when SDL3_image is available.
- Complete: render selection rings, movement targets, order lines, suppression, casualty, objective, and civilian-risk overlays from renderer-independent data.
- Complete: add fallback markers for missing sprite assets or unavailable SDL3_image.
- Continue: add finalized sprite art and packed runtime atlases when the art pass settles.

### Milestone C: 2003 Scenario Data

- Complete: create the first Market / Commercial Streets scenario data file.
- Complete: load controller slots, factions, forces, units, civilians, objectives, map metadata, and asset references from data.
- Complete: add parser/validation tests.
- Continue: expand the format only when new gameplay systems need new fields.

### Milestone D: Playable Contact

- Complete: add visible fire/suppression/casualty feedback records and overlays.
- Complete: add hidden-contact state, reveal checks, and suspected-contact overlays.
- Complete: add suspected-danger and false-contact records.
- Complete: add first civilian-risk scoring and a non-combatant proximity/fire constraint.
- Complete: add a basic opposing AI response.
- Complete: turn the first contact systems into scenario outcome scoring and after-action text.
- Continue: make uncertainty influence AI and player-facing commands rather than only reports/overlays.

### Milestone E: Demo Polish

- Complete: add deterministic after-action summary data and headless transcript output.
- Complete: add first deterministic debug/replay transcript lines and balance assertions.
- Continue: add player-facing briefing and after-action UI presentation.
- Continue: broaden deterministic replay/debug logging into replayable command/event files.
- Package one macOS-first smoke-tested build.
- Document how to build, run, test, and regenerate runtime assets.

## Immediate Next Steps

1. Convert debug/replay lines into an explicit replay event file format with versioned records.
2. Add AI behavior that reacts to suspected-danger contacts without treating them as confirmed units.
3. Add player command affordances for investigate/search and cautious movement near suspected danger.
4. Add objective-state and score presentation to the SDL shell when SDL3 is available.
5. Add scenario data fields for objective labels and score weight overrides.
6. Add a longer AI-only balance CTest or scripted scenario that reaches, contests, or resolves the objective.
7. Keep all manifest/scenario/render/AI changes covered by CTest, including missing asset and invalid-reference failures.

## Quality Bar

- Every new rule gets a deterministic test.
- Every manifest or scenario file gets validation.
- Missing art should produce a readable fallback, not a crash.
- Source assets remain unmodified.
- Runtime assets are reproducible.
- The demo must be playable at unit scale while preserving meaningful soldier-level consequences.
