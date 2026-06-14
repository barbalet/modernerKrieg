# Chapter 2: The C Contract

`modernerKrieg` is meant to be reached through C. That sounds plain, but it is
the most important architectural sentence in the project. C is the language
that both the headless tools and the native wrappers can agree on. SwiftUI on
Mac, a future Windows interface, tests, CI, and replay validators can all call
the same engine boundary.

The public contract has two main faces:

- [`../engine/core/include/mk_core.h`](../engine/core/include/mk_core.h), the
  simulation contract;
- [`../engine/demo/include/mk_demo.h`](../engine/demo/include/mk_demo.h), the
  native-wrapper session contract.

The first is the deep model. The second is the practical surface that a platform
frontend should start with.

## Result Codes And Bounded State

The core uses `mk_result_t`:

```c
typedef enum {
    MK_OK = 0,
    MK_ERROR_INVALID_ARGUMENT = -1,
    MK_ERROR_CAPACITY = -2,
    MK_ERROR_NOT_FOUND = -3,
    MK_ERROR_INVALID_DATA = -4
} mk_result_t;
```

That compact shape is valuable. A platform wrapper does not need exception
bridging or ownership gymnastics to know whether a command succeeded. The
engine can reject bad inputs and leave the caller with a stable reason class.

The header also defines explicit capacities: maximum units, soldiers per unit,
civilians, contact reports, terrain zones, gameplay levels, building features,
regions, topology nodes, portals, semantic zones, and route steps. Those caps
are not an admission that the model is small. They are the way the demo remains
predictable while it becomes richer.

An urban battle can fail in many ways. A wrong pointer should not be one of
them. Fixed-size arrays and opaque handles make the core easier to reason about
under tests and easier to bind from Swift.

## Sides, Controllers, And Orders

The side model is deliberately explicit:

- `MK_SIDE_NEUTRAL`
- `MK_SIDE_PLAYER`
- `MK_SIDE_OPFOR`
- `MK_SIDE_CIVILIAN`

The controller model is separate:

- no controller;
- human;
- scripted AI;
- tactical AI;
- observer.

This lets the project run the same battle with different command sources. A
human may command the player side in the Mac frontend. A tactical AI may command
both armed sides in `mk_ai_battle`. Civilians may remain observer-controlled in
one scenario and later receive a dedicated civilian AI cadence in another.

Orders are also explicit: hold, move, assault move, fire, suppress, overwatch,
breach, rally, withdraw, and investigate. The AI layer should emit these same
orders. A human click should become one of these same orders. Replay should
record and restore these same orders. That sameness is the contract.

## Soldiers Inside Units

The header's soldier-level types are where the project steps beyond a simple
unit-counter model:

- role, including rifleman, leader, machine gunner, RPG, marksman, engineer,
  medic, drone operator, and civilian;
- stance, including standing, crouching, and prone;
- wound state, from none through killed;
- fire mode and ammo kind;
- training, suppression, stress, exposure, and cooldown fields in the wider
  model.

The player will usually command a unit. The simulation still needs individual
soldiers because a patrol is not damaged in the abstract. A leader wounded, a
machine gunner pinned, or a breacher exposed near a door all have different
consequences for the next decision.

The C contract lets the engine hold those details without requiring the UI to
become a soldier database. Snapshots can expose enough for inspection and
rendering. The rules remain in C.

## Civilians In The Contract

Civilians have their own side and their own state model:

- sheltering;
- fleeing;
- frozen;
- following instructions;
- evacuated;
- wounded;
- dead.

They also have intent:

- shelter;
- flee;
- evacuate;
- follow instructions;
- freeze;
- assist group.

That is a strong design choice. Civilians are not hidden counters inside the
score system. They become actors the engine can update, route, stress, wound,
evacuate, and report. This is essential for Mosul. A game about a dense city
cannot treat civilians as background art and still claim to model tactical
restraint.

![Civilian state sheet](../assets/shared/source/sprite_sheets/19_civilian_states_topdown_128.png)

## The Demo Session API

The native-wrapper API begins with an opaque session:

```c
typedef struct mk_demo_session mk_demo_session_t;
```

A frontend can create and destroy a session, load the default Mosul scenario,
load a specific scenario file, toggle AI-only stepping, step fixed ticks, query
a snapshot, query a summary, and collect performance counters.

It can also collect draw commands. This is the key to platform-native
rendering. The C side does not need to draw through SDL, Metal, CoreGraphics,
or Direct2D. It can describe what should be drawn:

- level images;
- units;
- civilians;
- soldiers;
- objectives;
- contacts;
- overlays;
- traffic vehicles.

The frontend decides how to render the commands. The simulation decides what
the commands mean.

## Picking And Commands

The demo API supports screen picking:

- units;
- civilians;
- contacts;
- objectives;
- terrain;
- topology nodes;
- topology portals;
- semantic zones;
- traffic vehicles.

That list is revealing. The intended UI is not only a board with counters. It
is an inspector for the city. A user should eventually click a roof access
point, a breached door, a civilian group, an unresolved contact, or a danger
zone and see meaningful state.

Picking returns stable ids, labels, sides, map positions, and screen positions.
That makes it possible for native UI to remain thin. Selection and move orders
can flow back through C without the UI reimplementing coordinate conversion or
route legality.

## Debug, Audit, And After-Action Text

`mk_demo.h` also exposes after-action, audit, session debug, and topology debug
text. This is a gift to future debugging. A native app can show or export this
information. CI logs can capture it. A failed AI-only run can be examined
without reproducing a visual session.

The audit report is especially practical. It tracks counts for levels,
features, regions, topology nodes, portals, semantic zones, objectives, units,
civilians, missing level image paths, blocked or unsafe portals, breached
portals, searched portals, route failures, and warnings.

Those are exactly the categories that tend to break in a data-heavy tactical
engine. The public API makes them first-class.

## What Should Not Leak Across The Boundary

A platform wrapper should not inspect private engine memory. It should not
parse Mosul scenario files on its own. It should not decide whether a civilian
is safe, whether a portal can be crossed, or whether a target is visible. It
should ask the session API.

Likewise, the core should not include platform headers, open windows, upload
textures, or ask SwiftUI what the current selection is. The C boundary is the
meeting place. Each side should arrive with its own job cleanly done.

## The Practical Reading Rule

When changing the project, read the public headers first. If a new gameplay
fact must be visible to the UI, AI, replay, tests, and debugging, it probably
belongs in the C contract. If it is only a platform presentation preference, it
belongs in the native wrapper.

The contract should grow carefully, but it should grow. Mosul's complexity will
not fit behind a small unit-position API. The trick is to expose stable facts,
not private implementation.

## ABI And Snapshot Discipline

The native-wrapper contract should be treated as a small ABI, even while the
project is still young. That means public structs should grow intentionally and
callers should not depend on private layout. If a frontend needs a fact, prefer
adding a snapshot, summary, draw command, pick result, audit field, or debug
string over letting it reach into core internals.

Snapshots should answer "what is true now?" Draw commands should answer "what
should be presented now?" Audit/debug text should answer "why does this state
look this way?" Keeping those questions separate makes the interface easier to
bind from Swift and easier to port later.

## Frontend Checklist

A native frontend should be able to start with this sequence:

1. Create `mk_demo_session_t`.
2. Load the default scenario or a scenario path.
3. Fit the board to the current window.
4. Step fixed ticks.
5. Query summary and snapshot.
6. Collect draw commands.
7. Load asset paths into platform image resources.
8. Use C picking for mouse or touch input.
9. Issue orders through C.
10. Display after-action, audit, and debug text when needed.

If a frontend cannot do this without parsing scenario files or redoing gameplay
queries, the demo API is missing something.
