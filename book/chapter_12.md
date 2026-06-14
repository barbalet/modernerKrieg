# Chapter 12: Extending The Demo

The first 100-cycle C plan brought the repository to a strong engine-shaped
foundation: data-backed scenario loading, multi-level gameplay area parsing,
topology, civilian state, AI-only battles, replay, renderer-independent draw
commands, native-wrapper session API, asset validation, and CI checks.

The next work should deepen that foundation without reversing the dependency
direction. Add detail to C. Add content through data. Add presentation through
native wrappers. Keep the rules in one place.

## Adding A Scenario

A new scenario should begin as data under [`../game/mosul/scenarios/`](../game/mosul/scenarios/).
It should name:

- format, id, name, seed, briefing;
- score thresholds and weights;
- after-action text;
- asset manifests;
- map size and fallback terrain;
- controllers, factions, and forces;
- spawn zones;
- unit templates;
- civilian archetypes and groups;
- objectives;
- any scenario-specific smoke-test expectations.

Then add validation. A scenario that cannot be loaded by headless tools is not
ready for native UI.

## Adding Map Detail

Map detail should enter through building-level or topology data, not through
frontend-only geometry. New rooms, doors, windows, roofs, caches, shelters,
exits, and danger zones should get stable ids.

A good map change should answer:

- does this feature affect movement?
- does it affect line of sight?
- does it affect cover?
- does it affect civilian behavior?
- does it affect AI choices?
- should it appear in replay or debug text?
- should it be pickable in the native UI?

If the answer is yes, it needs a C-side representation.

## Adding Assets

New art should follow the source, manifest, runtime separation:

- put raw references and source art under `assets/mosul/source/`;
- update manifests under `assets/mosul/manifests/`;
- generate or copy runtime products under `assets/mosul/runtime/`;
- update `assets/ASSETS.md` when the review catalogue changes;
- add or update C validation tests.

When adding sprites, preserve stable runtime ids. Replays and scenarios should
not break because an asset was reorganized. If an id must change, treat it as a
schema/content migration.

## Adding Rules

A rule belongs in C if it changes:

- legality of an order;
- movement, line of sight, or cover;
- soldier, unit, civilian, vehicle, contact, portal, or objective state;
- AI decisions;
- replay output;
- scoring or after-action text;
- native UI inspection.

The native UI can present the rule, but the core should own it. This is the
discipline that keeps the Mac and Windows versions from drifting apart.

## Adding AI

AI should continue to issue ordinary orders and intent updates. It should not
get special state mutations. New AI behavior should come with smoke scenarios
and headless expectations.

Useful future AI additions include:

- route-aware patrol bounding;
- civilian-aware fire discipline;
- role-aware search and breach;
- opfor withdrawal and concealment;
- civilian group cohesion;
- rooftop threat selection;
- cache defense;
- medical or casualty response.

Each behavior should leave evidence in logs, replay, snapshots, or debug text.

## Adding Native UI

The Mac and Windows frontends should start from
[`../engine/demo/include/mk_demo.h`](../engine/demo/include/mk_demo.h). The
native UI should:

- create a session;
- load the Mosul scenario;
- step fixed ticks;
- query summaries and snapshots;
- collect draw commands;
- load images from command asset paths;
- pick screen positions through C;
- issue orders through C;
- show after-action, audit, and topology debug text when useful.

The frontend can make the experience feel modern. It can use platform-native
menus, panels, gestures, rendering, and accessibility. It should not become a
second engine.

## Updating This Book

Update the book when:

- a new public C concept is added;
- a scenario format changes;
- map/topology rules change;
- civilian AI changes meaningfully;
- new asset classes or runtime products appear;
- native-wrapper responsibilities change;
- CI or replay workflows change;
- art plates are replaced by final unified artwork.

The book is not a marketing page. It is a maintainer's map. It should remain
close enough to the source that a reader can move from prose to code without
losing the thread.

## What To Protect

As the demo becomes more complete, protect these ideas:

- deterministic C simulation;
- data-backed Mosul content;
- stable asset manifests;
- topology as gameplay truth;
- civilians as agents;
- AI-only battles as evidence;
- replay as memory;
- native UI as presentation and command surface;
- graphics as atmosphere and contract.

The project can become larger without becoming vague. That is the whole point
of the current shape.

## The Last Word

The explicit engine is arrays, ids, routes, statuses, manifests, tests, and
draw commands. The esoteric engine is attention: to uncertainty, civilians,
fear, restraint, scale, and the difference between what is visible and what is
known.

`modernerKrieg` should keep both. The C code should be sober enough to test.
The art should be vivid enough to remember. The demo should let the native
frontends show Mosul without making them carry Mosul's rules.

## A Change Checklist

Before merging a meaningful engine or content change, check:

- does the C API still describe the new behavior?
- do scenario and asset validators catch broken references?
- do smoke scenarios cover the new rule or data shape?
- does AI-only play still settle?
- does replay still validate?
- does the native-wrapper API expose enough for UI?
- does the book need a source anchor or art plate update?

This checklist is deliberately plain. The project will grow, but its review
habits should stay simple enough to use every time.
