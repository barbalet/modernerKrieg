# Chapter 4: The Multi-Level City

The 2003 Market / Commercial Streets demo is not a flat board. The repository
contains a JSON-backed multi-level gameplay area and a runtime stack of four
large PNG level plates. This is the foundation for interiors, roofs, stairs,
elevated line of sight, blocked movement, search, breach, and civilian shelter.

The building-level manifest is:
[`../assets/mosul/manifests/market_commercial_streets_2003_building_levels.json`](../assets/mosul/manifests/market_commercial_streets_2003_building_levels.json).

The topology manifest is:
[`../assets/mosul/manifests/market_commercial_streets_2003_topology.json`](../assets/mosul/manifests/market_commercial_streets_2003_topology.json).

## The Level Stack

The runtime level images are:

- ground level;
- roofs and second floors;
- upper floor;
- roof access.

![Ground level](../assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_01_ground.png)

The images are 7,000 px by 7,000 px at 14 px/m. That gives a 500 m by 500 m
world. The level art is not only visual context. It is the substrate for
authored features, building regions, topology nodes, and renderer draw order.

The manifest records level ids, indices, elevation, PNG paths, alpha mode, and
default movement/LOS behavior. Those records let the C asset layer validate
that the art stack exists before a scenario uses it.

## Building Features

The building-level manifest describes authored rectangles for:

- walls;
- doors;
- windows;
- breach holes;
- stairs;
- roof edges.

Each feature can block line of sight, block movement, allow line of sight, or
allow movement. This is the first bridge between ink and rules.

A wall blocks line of sight and movement. A window may allow sight but not
movement. A door may override a wall for movement. A breach hole may become a
dangerous opening. A stair is not only art; it is a vertical relationship.

The important thing is that the engine reads those facts as data. A native UI
does not decide that a window is see-through because it looks like a window.
The manifest says what it does.

## Building Regions

Building regions describe larger authored areas with storey counts and roof
level references. They are the first pass at "this mass of pixels is a
building." Regions become useful when the engine needs to know whether a unit
is inside, on a roof, near a roof edge, in a building with multiple storeys, or
in a place civilians might shelter.

Regions should eventually carry dynamic state:

- enterable;
- locked;
- breached;
- searched;
- occupied;
- roof controlled;
- civilian shelter;
- damaged or dangerous.

Those dynamic states belong in the core. The manifest supplies stable
authored identity. The simulation records what happened.

## Topology Nodes

The topology manifest turns rectangles into a tactical graph. Nodes can
represent streets, alleys, shops, courtyards, roofs, stairwells, caches,
shelters, and blocked buildings. Nodes have stable ids, kinds, level ids,
region ids, labels, bounds, and enterable flags.

This graph is where the demo becomes an urban simulation rather than a
position-only skirmish. A unit does not merely move from x/y to x/y. It can
move from a street node through a doorway into a shop row, up stairs to a roof,
or around a blocked alley.

Stable topology ids are also essential for debugging. A replay line that says a
unit entered `hotel_roof_access` is more useful than one that says a unit moved
to `(331.2, 102.8)`.

## Portals

Portals connect nodes. They can be bidirectional or one-way, horizontal or
vertical, open or blocked, safe or unsafe, cheap or costly. Doorways, windows,
breach holes, archways, stairs, ladders, roof edges, street crossings, and
rubble passages can all be portals.

This is where many urban rules should live:

- movement cost;
- blocked or unsafe movement;
- breach state;
- searched state;
- vertical transition;
- door/window behavior;
- route failure reasons.

The asset validator rejects duplicate ids, invalid references, one-way mistakes,
impossible vertical links, and unreachable enterable graph fragments. That is
exactly the kind of strictness this map needs.

## Semantic Zones

Semantic zones name places that are meaningful beyond movement:

- civilian shelters;
- evacuation exits;
- market crowds;
- caches;
- overwatch roofs;
- search objectives;
- restricted fire lanes;
- danger areas.

These zones should influence AI and scoring. A civilian shelter should draw
civilians under threat. A restricted fire lane should raise restraint pressure.
A cache zone should give search orders a reason to exist. An overwatch roof
should matter to both line of sight and threat evaluation.

## Derived Tactical Products

The current C core derives first-pass tactical products from validated
building-level and topology data:

- movement blockers;
- LOS blockers;
- navigation costs;
- cover hints;
- sampled gameplay-area LOS traces;
- deterministic topology routes.

This means no platform frontend has to do image processing to decide gameplay.
The native wrapper may draw the level images. It should not use pixels as
private collision truth.

## The Esoteric Part Of The Map

A city is not only geometry. It is uncertainty compressed into walls. A street
corner changes who can see. A roof changes who feels exposed. A shop interior
changes whether a civilian is protected or trapped. A door changes whether an
order is cautious or reckless. A window changes what a contact report means.

The multi-level gameplay area is where those meanings can become deterministic
state. The art gives the player a city. The topology gives the engine a city.
The demo works only when those two cities are the same place.

## Debugging The City

A topology bug should be debugged with names, not only with pixels. Useful
debug text should name:

- the source level id;
- the topology node id;
- the portal id and state;
- the semantic zone id;
- the first blocking feature id for LOS;
- the route step where movement failed;
- whether a vertical link was involved.

The demo API already exposes topology debug text. Keep expanding that surface
as the map gains more authored detail.

## Interior Work Still Ahead

The current topology is a strong first pass, but a fully fledged demo should
continue toward richer interiors:

- shop rows with individual rooms or bays;
- courtyards and rear exits;
- stairs that connect explicit floor nodes;
- roofs with edge exposure;
- blocked buildings that can become enterable later;
- civilian shelters with capacity and safety ratings;
- cache zones with search state;
- windows that support sight but not normal movement.

Each addition should be small, stable, and validated. The city should grow as a
graph the engine understands, not as a pile of special cases.
