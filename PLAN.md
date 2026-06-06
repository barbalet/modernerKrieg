# modernerKrieg Plan

`modernerKrieg` is the new portable tactical wargame engine for the MOSUL public demo. It should start fresh as a CMake project with a portable C core. SDL3 is the current experimental app shell; if it proves workable, it will be used, and if not, the contingency is a new SwiftUI GUI over the same tactical model. `derZweiteWeltkrieg` remains a proven design reference rather than a submodule or runtime dependency.

The guiding idea is simple: keep the game playable at unit scale, but simulate enough detail inside each unit that soldiers, weapons, casualties, suppression, civilians, and urban terrain all matter.

## Direction

Build a clean engine in this repository.

Use `derZweiteWeltkrieg` for:

- architecture lessons from its C rules core and thin presentation layer
- examples of deterministic tests around tactical rules
- useful ideas for mission state, terrain snapshots, objectives, morale, and damage resolution
- reference code only when it is worth porting deliberately

Do not use `derZweiteWeltkrieg` as:

- a git submodule
- a package dependency
- a naming or era model for the new codebase
- the authority for modern-era force, weapon, civilian, intelligence, drone, IED, or urban-control systems

If code is copied rather than rewritten, copy it intentionally, preserve license/attribution, rename it into the new domain, and add tests at the same time.

## Engine Goals

- Run on macOS, Windows, and Linux.
- Put the simulation, rules, AI, replay, and scenario logic in portable C.
- Test SDL3 for windowing, input, audio, and cross-platform runtime services; keep the core portable enough for a SwiftUI GUI contingency.
- Use CMake as the primary build system.
- Keep platform, renderer, tools, and content above the core simulation.
- Support deterministic headless tests without opening a window.
- Make Mosul the first vertical slice and the primary design pressure.

## Proposed Repository Shape

```text
modernerKrieg/
  CMakeLists.txt
  PLAN.md
  README.md
  assets/
    mosul/
      source/
      sprites/
      atlases/
      maps/
      references/
  docs/
    mosul_design.md
    engine_architecture.md
    asset_pipeline.md
  engine/
    core/
      include/
      src/
    platform/
      sdl3/
    render/
    tools/
  game/
    mosul/
      data/
      scenarios/
      src/
  tests/
    core/
    mosul/
```

`engine/core` must not depend on SDL, graphics APIs, operating-system UI, or Mosul-specific content. The Mosul game module can depend on the engine. The SDL application can depend on both.

## Simulation Model

The player should mostly command units, but the engine should know the important soldiers inside each unit.

A unit owns:

- command identity, faction, side, morale state, training quality, and current order
- formation center, facing, movement intent, cohesion radius, and cover posture
- communication state, suppression level, fatigue, and command disruption
- soldier records for the people who make the unit real

A soldier owns:

- role, weapon, ammunition, carried equipment, and optics/sensor capability
- health, wound state, casualty state, stress, and suppression contribution
- local offset inside the unit, stance, facing, and exposure
- line-of-sight participation, fire participation, reload/cooldown state, and ability to move

This supports unit-based play while allowing soldier-level outcomes:

- a squad can lose its machine gunner without being destroyed
- a breacher, medic, drone operator, or marksman can matter
- soldiers can be pinned in different parts of a building
- infantry can become separated from a vehicle
- civilians can occupy the same urban space without being treated as scenery

## Mosul Demo Scope

The first playable demo should be a small urban block, not the whole city.

Current initial scenario:

- 2003 Market / Commercial Streets, after Mosul's fall during Operation Iraqi Freedom
- U.S. Army conventional infantry associated with the 101st Airborne Division period in Mosul
- regime remnants, irregular fighters, weapons looters, early insurgent cells, and disorder around civic/commercial spaces
- civilians present as protected non-combatants and movement/fire constraints
- shops, market lanes, streets, courtyards, rooftops, upper floors, rubble, checkpoints, and weapons-cache/search objectives

Core demo loop:

1. Select a unit.
2. Give an order: move, assault move, fire, suppress, overwatch, breach, rally, or hold.
3. Resolve soldier-level visibility, cover, shots, wounds, suppression, and morale.
4. Advance the opposing AI response.
5. Score the local objective with civilian-harm and force-preservation consequences.

The demo is successful when it proves that modern urban combat feels meaningfully different from a WWII tabletop reskin.

## First Engine Milestones

### Milestone 0: Skeleton

- Add top-level CMake project.
- Add a pure C core static library.
- Add a headless test executable.
- Add an SDL3 app target that opens a window and runs a fixed-step loop.
- Add CI-friendly commands for configure, build, and test.

### Milestone 1: Core State

- Define game, map, terrain, side, faction, unit, soldier, weapon, and scenario structs.
- Add deterministic random seed handling.
- Add serialization-friendly state snapshots.
- Add tests for unit creation, soldier rosters, movement intent, and scenario loading.

### Milestone 2: Tactical Board

- Render a top-down board with pan/zoom.
- Draw units, soldier offsets, cover, terrain, objectives, and selection state.
- Support mouse selection and basic unit orders.
- Keep rendering as a view of engine state, not the owner of game rules.

### Milestone 3: Urban Combat

- Add line-of-sight and cover checks.
- Add small arms fire, machine-gun suppression, RPG direct fire, and basic wounds.
- Add morale/suppression effects at unit and soldier levels.
- Add hidden enemy contact markers and simple reveal logic.

### Milestone 4: Mosul Systems

- Add civilians and civilian-risk scoring.
- Add simple IED detection/resolution.
- Add breach/entry actions for buildings.
- Add rooftops, building interiors, and alley/road movement costs.
- Add first-pass AI for defending, ambushing, withdrawing, and counterattacking.

### Milestone 5: Playable Mosul Demo

- Package one polished scenario.
- Use Mosul-specific art assets and readable UI.
- Add scenario briefing and after-action summary.
- Add deterministic replay logging for debugging and balancing.
- Build and smoke-test on macOS first, then Windows and Linux.

## Art Asset Direction

The Mosul private brief already has a strong art start: line art maps, combatant plates, weapon plates, vehicle plates, 128 px top-down sprite sheets, and 2003 Market / Commercial Streets map layers. Treat those as source art and organize them into an engine-ready asset pipeline.

Initial asset folders:

```text
assets/mosul/source/line_art/
assets/mosul/source/sprite_sheets/
assets/mosul/sprites/characters/
assets/mosul/sprites/vehicles/
assets/mosul/sprites/weapons/
assets/mosul/atlases/
assets/mosul/maps/
assets/mosul/references/
```

Do this first:

- copy the Mosul README art index into `docs/asset_pipeline.md`
- bring source PNGs across without destructive edits
- keep 128 px sheets as high-detail tactical sprites
- do not reintroduce 64 px combatant renderings for the current demo
- define atlas metadata before slicing sheets
- define a consistent pivot point, facing convention, scale, and faction/role naming scheme

Suggested sprite naming:

```text
us_army_rifleman_128_n.png
us_army_squad_leader_128_n.png
us_army_automatic_rifleman_128_n.png
us_army_grenadier_128_n.png
us_army_medic_128_n.png
opposing_regime_rifleman_128_n.png
opposing_rpg_gunner_128_n.png
opposing_weapons_looter_128_n.png
civilian_adult_128_n.png
humvee_128_n.png
```

Suggested atlas metadata:

```json
{
  "id": "mosul_characters_128",
  "source": "assets/mosul/source/sprite_sheets/12_us_ally_troops_topdown_128.png",
  "tile_width": 128,
  "tile_height": 128,
  "pivot": { "x": 64, "y": 64 },
  "facing": "north_up",
  "frames": [
    {
      "id": "us_army_rifleman",
      "faction": "us_army_2003",
      "role": "rifleman",
      "x": 0,
      "y": 0
    }
  ]
}
```

Art should serve readability first. The player must be able to distinguish:

- U.S. Army patrol soldiers, local security, regime remnants, irregular fighters, weapons looters, and civilians
- rifleman, squad leader, automatic rifleman, grenadier, marksman, engineer/breacher, medic, vehicle crew, RPG gunner, machine gunner, and looter/criminal threat
- infantry, Humvees, trucks, technicals, engineering vehicles, and static weapons
- open street, market lane, shopfront, rubble, interior, rooftop, courtyard, obstacle, breach point, checkpoint, and suspected weapons-cache or IED zone

## Data Direction

Prefer data files for Mosul content rather than hard-coded scenario constants.

Start with simple JSON or a compact custom text format for:

- factions
- weapons
- soldier templates
- unit templates
- terrain definitions
- map layouts
- scenario objectives
- AI plans

The C core should load validated data into stable runtime structs. Tests should cover parsing and reject invalid scenario files.

## Design Principles

- The engine should model uncertainty, not omniscience.
- Civilian presence should change player decisions without becoming exploitative spectacle.
- Air support, artillery, and heavy weapons should be powerful but constrained by visibility, ROE, terrain, and civilian risk.
- Urban combat should reward reconnaissance, suppression, breach timing, overwatch, and force preservation.
- The game should be historically grounded without becoming a faction power fantasy.
- Every major rule should have a deterministic test before it becomes hard to change.

## Immediate Next Steps

1. Create the CMake + SDL3 skeleton.
2. Add `engine/core` with deterministic game-state and unit/soldier structs.
3. Add the first headless tests.
4. Import Mosul source art into `assets/mosul/source`.
5. Write the first atlas metadata file for 128 px combatant sheets.
6. Render a board with placeholder terrain and real Mosul unit sprites.
7. Build the first 2003 Market / Commercial Streets scenario.
