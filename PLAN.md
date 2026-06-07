# modernerKrieg Plan

`modernerKrieg` is the portable tactical engine and runtime for the public MOSUL demo. The engine should now be developed as a PNG-backed tactical loader and renderer with deterministic gameplay behind it: the player sees authored Mosul map art, sprites, and tactical markers, while the portable C core owns movement, line of sight, fire, suppression, casualties, civilian risk, scenario state, AI, and replayable outcomes.

The first public target is the 2003 Market / Commercial Streets demo. `derZweiteWeltkrieg` remains useful design memory for deterministic rules, tests, and thin presentation layers, but it is not a dependency, submodule, naming model, or era model for MOSUL.

## Current Baseline

- CMake project, portable C core, Mosul game module, renderer-independent board-view code, and headless tests exist.
- The core already supports deterministic game state, unit selection, orders, movement ticks, line of sight, cover checks, unit fire, ammo spend, wounds, casualties, suppression, morale state changes, hidden-contact reveals, suspected/false contact reports, civilian-risk updates, objective control, scoring thresholds, and after-action summaries.
- The core state model now includes controller slots, force records, command identities, map tiles with elevation/cover/movement costs, richer weapon/ammunition metadata, richer soldier state, standalone civilian records, contact reports, hidden unit fields, and snapshot/load support for those containers.
- The render layer already projects map, terrain, objectives, units, soldier offsets, selection, movement targets, fire lanes, suppression, hidden contacts, casualties, civilian-risk overlays, visible unit order-status overlays, and interaction-zone overlays into screen space.
- Deterministic headless runners exist for smoke tests, replay generation/validation, and AI-vs-AI/autoplay work.
- CMake presets exist for default, headless, and strict warning-as-error builds.
- Asset pipeline documentation, the first Market / Commercial Streets map manifest, the first 2003 sprite manifest, the first marker manifest, and C manifest validation tests exist.
- The 2003 Market / Commercial Streets scenario now loads from a validated `.mkscenario` data file, with a C fixture retained for parity tests.
- `mk_headless_run` can load an explicit scenario path, override the seed, run for a fixed tick budget, suppress console output, write a transcript with contact/risk counters, briefing, debug lines, versioned replay/event files, balance expectations, and after-action output, and run both non-civilian tactical sides under basic AI.
- `mk_replay_validate` can validate versioned `.mkreplay` event files, assert final result/outcome fields, and print compact tick-range playback summaries.
- The first runtime map overview exists at `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`.
- The SDL prototype has been removed after evaluation. No SDL app target, SDL package target, or SDL development dependency remains in the active build.
- The next launchable interfaces should be platform-native shells over the same C libraries: a Mac frontend first, then a Windows frontend.
- Source art for the 2003 demo is imported under `assets/mosul/source/`.
- Public source art includes line-art references, 128 px top-down sprite sheets, source-angle weapon sprites, and Market / Commercial Streets map/layer assets.
- The first 100-cycle public-demo plan is complete; remaining work is now final art replacement, deeper interaction rules, cross-platform packaging validation, and playtesting.

## Runtime Shape

The playable app should behave primarily as a PNG loader and renderer with gameplay state behind it.

- Load map art, tactical sprites, markers, and metadata from manifests.
- Render the map as image layers or generated tiles, not as colored terrain rectangles.
- Render units from sprite metadata, facing, role, side, stance, casualty state, and selection state.
- Render tactical overlays for orders, routes, line of sight, objectives, suppression, hidden contacts, civilian risk, breach/search points, and rooftop access.
- Keep native frontend code as presentation and input handling only.
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

`engine/core` must not depend on frontend frameworks, graphics APIs, operating-system UI, or Mosul-specific art files. The Mosul game module can depend on the engine. The app/frontend can depend on both.

## Asset Pipeline

Source assets stay unmodified. Runtime files should be generated, validated, and rebuildable.

Current source asset groups:

- `assets/mosul/source/line_art/`: Mosul context, combatant, weapon, vehicle, and urban tactics plates.
- `assets/mosul/source/sprite_sheets/`: 128 px combatant, stance, vehicle, and weapon source sheets.
- `assets/mosul/source/sprite_sheets/source_angles/infantry_128/`: approved source-angle infantry PNGs for 16 demo roles across standing, crouch, prone, wounded, and dead states.
- `assets/mosul/source/sprite_sheets/source_angles/weapons_128/`: approved source-angle weapon PNGs.
- `assets/mosul/source/sprite_sheets/source_angles/vehicles_1024/`: approved source-angle vehicle PNGs across intact, damaged, and destroyed states.
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/`: Market / Commercial Streets map previews, layer manifest, and source map layers.
- `assets/mosul/source/notes/`: provenance and demo asset selection notes.
- `assets/mosul/runtime/sprites/rendered/`: generated runtime-facing sprites, currently 896 PNGs.

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

The SDL experiment is closed. The engine should now favor platform-native presentation while preserving a shared C simulation, asset, render-projection, AI, replay, and validation core.

- Build the Mac frontend as the first platform-native interface over the existing C libraries.
- Build the Windows frontend against the same C-facing contracts rather than forking gameplay logic.
- Keep command-line AI battles, replay validation, and CTest as the fastest diagnostic surface.
- Do not put rules, scenario decisions, or asset interpretation exclusively in frontend code.

## Completed Foundation

The following baseline work is complete and should be preserved while the plan pivots to PNG-backed public-demo work:

- Milestone 0 skeleton: CMake, an experimental app shell since removed, headless runner, fixed-step loop, test helper layer, build presets, CI notes, asset folder stubs, core/frontend-boundary smoke test, and architecture/third-party docs.
- Milestone 1 core-state first half: stable containers for controllers, forces, command identities, terrain tiles, civilians, units, soldiers, weapons, objectives, snapshots, scenario loading, and validation.
- Asset/data foundation pass: source-safe asset pipeline docs, map manifest, sprite manifest, manifest parser/validator, manifest CTests, frontend manifest handoff, and 2003 scenario entry point.
- Current tests cover deterministic RNG, scenario loading, scenario data validation, snapshots, movement, LOS, firing, suppression, board-view transforms, fixed-step runs, asset manifests, and the portable-core boundary.

## Development Cycle Ledger

Cycle accounting starts from the current checked-in PNG-backed Mosul demo plan state, after the completed foundation above. Earlier foundation work is preserved as baseline work and is not renumbered into this ledger.

- Ledger baseline date: 2026-06-06.
- Planned cycle budget: 100 cycles.
- Planned cycle shape: 5 milestones x 20 cycles each.
- Completed cycles in this ledger: 100.
- Current cycle: 100.
- Remaining planned cycles: 0.
- Next cycle batch: complete; start a new ledger for post-plan work.

When a development batch is completed, increment `Completed cycles in this ledger`, advance `Current cycle`, reduce `Remaining planned cycles`, and update `Next cycle batch`. Add one log row describing the cycle range, milestone focus, completed work, and verification.

| Date | Cycles Completed | Current Cycle | Remaining Cycles | Milestone Focus | Notes |
| --- | ---: | ---: | ---: | --- | --- |
| 2026-06-06 | 0 | 0 | 100 | Baseline | Ledger added after the completed foundation pass; next work starts at cycles 1-10. |
| 2026-06-06 | 10 | 10 | 90 | Milestone C / Autoplay | Added the 2003 `.mkscenario` file, scenario parser, fixture parity and invalid-reference tests, default data-backed Mosul loader, and headless scenario/seed/tick/quiet/transcript controls. Verified default CTest and direct transcript run. |
| 2026-06-06 | 20 | 20 | 80 | Milestone B / Autoplay | Added marker metadata and validation, renderer-independent tactical overlays, SDL sprite/marker fallback rendering, copied the first runtime map overview, introduced basic AI order emission, and added AI-only headless smoke coverage. Verified default CTest and direct AI transcript run. |
| 2026-06-06 | 30 | 30 | 70 | Milestone D / Contact | Added contact reports for fire/reveal/civilian risk, hidden-contact scenario fields and reveal checks, civilian-risk updates from fire and proximity, fire/hidden overlays, smarter AI suppress/withdraw choices, and deterministic AI transcript assertions. Verified default CTest and direct AI transcript run. |
| 2026-06-06 | 40 | 40 | 60 | Milestone D/E / Outcome | Added persistent objective-control state, deterministic score math for objectives/civilian risk/casualties/time, after-action summary data, `mk_headless_run --aar`, player AI civilian-risk restraint, revealed-opfor withdrawal behavior, and deterministic tests for scoring/AAR/AI caution. Verified default CTest and direct AI-only AAR transcript run. |
| 2026-06-06 | 50 | 50 | 50 | Milestone D/E / Uncertainty + Replay | Added suspected-danger and false-contact records with confidence/terrain metadata, objective-control/suspected/false overlays, scenario briefing/AAR text and score thresholds, headless `--briefing`, `--debug-log`, `--expect-objective`, and `--expect-min-score`, plus CTest coverage for the new debug/balance path. Verified default CTest and direct briefing/debug/AAR transcript run. |
| 2026-06-06 | 60 | 60 | 40 | Milestone D/E / Replay + Commands | Added versioned headless replay/event files, investigate and assault-move command APIs, cautious investigate movement, contact picking for suspected/false reports, suspected-contact AI investigate/overwatch behavior, objective labels, scenario score-weight overrides, a compact AI-only control-balance scenario, and CTest coverage for replay output and balance expectations. Verified default CTest, strict warning-as-error CTest, and direct AI-only replay/transcript run. |
| 2026-06-07 | 70 | 70 | 30 | Milestone E / Replay Validation + Balance | Added standalone `.mkreplay` validation tooling, end-to-end replay validation CTest coverage with an invalid replay negative check, richer headless expectations for final outcome/contested objectives/civilian risk, and a contested civilian-risk AI-only smoke scenario. Verified default CTest, direct replay validation, and direct contested-risk balance run. |
| 2026-06-07 | 80 | 80 | 20 | Milestone E / SDL Presentation + Smoke | Added renderer-independent order-status overlays, explicit order marker manifest records, SDL score/objective/risk HUD bars, SDL order-status glyphs, `--smoke-frames`, an SDL dummy-video smoke CTest, and an Apple Silicon `default-arm64` preset for Homebrew SDL3. Verified `default-arm64` build and CTest, including the SDL smoke test. |
| 2026-06-07 | 90 | 90 | 10 | Milestone E / Replay Playback + Seed Sweeps | Added `.mkreplay` tick-range playback summaries, replay playback CTest coverage, AI battle seed-step sweeps, batch settlement/stall/worst-score expectations, a deterministic five-seed AI-only balance CTest, and matching Xcode scheme launch arguments. Verified direct seed sweep expectations, replay playback, default-arm64 CTest, strict warning-as-error CTest, and Xcode project build. |
| 2026-06-07 | 100 | 100 | 0 | Milestone E / Interaction Data + Package | Added first-pass breach/search, cache/search, and rooftop/stair terrain zones; renderer-independent interaction-zone overlays; SDL mission/status/AAR panels; SDL `--project-root`, `--scenario`, and `--ai-only` launch controls; an SDL smoke CTest using the new controls; and a macOS smoke-tested package target. Verified direct SDL AI-only smoke, package creation, default-arm64 CTest, strict warning-as-error CTest, and package smoke. |

Historical ledger rows can mention the removed SDL experiment. Current and future development should use native frontends over the portable C core.

## Milestones

### Milestone A: PNG Map On Screen

- Complete: add map asset metadata.
- Complete: load a Market / Commercial Streets PNG map source or generated runtime image.
- Complete: project it through the current board view with pan/zoom data for native frontends.
- Complete: preserve map-to-screen and screen-to-map picking.
- Keep headless tests passing.

### Milestone B: Real Unit Sprites

- Complete: add sprite metadata for the first U.S. patrol and one opposing cell.
- Complete: expose unit sprite choices from role/side/state for frontend rendering.
- Complete: render selection rings, movement targets, order lines, suppression, casualty, objective, and civilian-risk overlays from renderer-independent data.
- Complete: render visible unit order-status glyphs from renderer-independent overlay data.
- Complete: add fallback markers for missing sprite assets or unavailable image backends.
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
- Complete: make investigate/search resolve suspected-danger and false-contact reports, including reveal/no-threat outcomes.
- Complete: add first civilian-risk scoring and a non-combatant proximity/fire constraint.
- Complete: add a basic opposing AI response.
- Complete: turn the first contact systems into scenario outcome scoring and after-action text.
- Complete: make uncertainty influence first-pass AI and player-facing investigate commands rather than only reports/overlays.
- Continue: deepen uncertainty into search/breach/cache systems once those scenario locations exist.

### Milestone E: Demo Polish

- Complete: add deterministic after-action summary data and headless transcript output.
- Complete: add first deterministic debug transcript lines and balance assertions.
- Complete: broaden deterministic replay/debug logging into versioned replay/event files.
- Complete: add first player-facing investigate command affordance for suspected and false contacts.
- Complete: add replay validation tooling and invalid-replay coverage for versioned replay/event files.
- Complete: add first contested-objective and civilian-risk AI-only balance smoke coverage.
- Complete: add first score/objective/risk HUD, order-status glyphs, and app smoke-test prototype during the SDL evaluation.
- Complete: add replay playback for versioned `.mkreplay` event files.
- Complete: broaden AI-only balance coverage into seed sweeps with batch summary expectations.
- Complete: add scenario data for first-pass breach/search, cache/search, and rooftop/stair access points.
- Complete: add player-facing briefing and after-action UI presentation during the SDL evaluation.
- Complete: package one macOS-first smoke-tested prototype before the SDL path was retired.
- Complete: document how to build, run, test, and package the current runtime assets.

## Post-Plan Next Steps

1. Replace provisional source art with the uniform art pass and regenerate runtime products.
2. Deepen breach/search/cache/rooftop affordances into rules, commands, AI choices, and AAR scoring.
3. Build a native Mac frontend over the portable C contracts and use it to replace the retired SDL launch path.
4. Build a native Windows frontend against the same contracts.
5. Add a new post-plan cycle ledger for playtest, usability, and deployment work.
6. Keep all manifest/scenario/render/AI changes covered by CTest, including missing asset and invalid-reference failures.

## Quality Bar

- Every new rule gets a deterministic test.
- Every manifest or scenario file gets validation.
- Missing art should produce a readable fallback, not a crash.
- Source assets remain unmodified.
- Runtime assets are reproducible.
- The demo must be playable at unit scale while preserving meaningful soldier-level consequences.
