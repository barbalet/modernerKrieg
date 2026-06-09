# Chapter 8: Movement, LOS, And Urban Surfaces

Movement and line of sight are the mechanical center of the Mosul demo. If a
unit can walk through a wall, see through a blocked building, ignore roof
access, or route civilians across an impossible alley, the rest of the
simulation loses trust.

The current project has three coordinate worlds that must agree:

- world meters, used by the core and scenario data;
- source/runtime pixels, used by map images and authored rectangles;
- screen pixels, used by native wrappers through board projection.

The important rule is simple: gameplay decisions happen in world/topology
space, not in the platform renderer.

## World Meters

The demo map is 500 m by 500 m. Movement speeds, objective radii, terrain
bounds, unit positions, civilian positions, and route waypoints are all
expressed in world meters. This gives the engine a stable scale no matter how a
native wrapper zooms or pans.

The board view in
[`../engine/render/include/mk_board_view.h`](../engine/render/include/mk_board_view.h)
fits the map to a screen rectangle and converts between map and screen
coordinates. This layer is renderer-independent. It does not draw through a
windowing API. It describes projection.

## Pixel Scale

The level art uses 14 px/m. The building-level manifest records pixel width,
pixel height, pixels per meter, and origin convention. That lets the asset
layer translate authored rectangles into world-space features.

Pixel scale matters for review. Designers and artists can inspect the 7,000 px
plates. The engine can still reason in meters. The bridge between the two must
be stable and tested.

## Routes

The core has a compact route model with a maximum number of route steps.
Routes should be built from topology nodes and portals. Direct movement can
remain as a fallback or simple order, but serious urban behavior needs graph
movement.

For units, route state should capture:

- current node;
- destination node or position;
- waypoint list;
- failure reason;
- vertical transitions;
- movement cost;
- blocked portals.

For civilians, route state should also capture intent: shelter, flee, evacuate,
follow instructions, freeze, or assist group.

## Line Of Sight

Line of sight should use building features, levels, and topology. Coarse
terrain zones are useful but insufficient. Walls, doors, windows, breach holes,
roof edges, and elevation records all change what can be seen.

The C core already has sampled gameplay-area LOS traces. That is the right
early technique. A sampled trace can report the first blocking feature, the
level involved, and whether an opening overrides a blocker. Later precision can
improve, but the contract should stay: LOS is an engine query with debug
evidence.

## Cover

Cover is not just "inside terrain." A window, doorway, wall, rubble passage,
roof edge, interior, or vehicle can all provide different protection and
different exposure. The derived tactical products already include cover hints.

Cover should eventually distinguish:

- concealment from view;
- ballistic protection;
- elevation advantage;
- civilian shelter;
- restricted fire lanes;
- dangerous exposure while crossing.

That difference matters because suppressing a hidden shooter, crossing an open
street, and sheltering civilians inside a shop are not the same problem.

## Screen Picking

The native wrapper API can map screen positions back to gameplay picks:

- unit;
- civilian;
- contact;
- objective;
- terrain;
- topology node;
- topology portal;
- semantic zone;
- traffic vehicle.

This is a subtle but powerful piece of architecture. It means a Mac UI can
inspect the city through C state. A click on a roof access marker can identify
the portal. A click on a shelter zone can identify the semantic zone. The UI
does not need to maintain a parallel spatial index.

## Tactical Overlays

The board-view layer can project overlays for selection, movement route,
movement target, fire, suppression, casualty, objective, hidden contact,
civilian risk, suspected contact, false contact, objective control, order
status, breach/search, rooftop access, and search/cache.

Those overlays are not final art. They are a renderer-independent vocabulary.
They tell native frontends what needs visual explanation. The frontend can draw
them with a platform-native look, but it should not invent new gameplay
meanings behind them.

## The Surface Is Not Flat

The esoteric mistake in urban tactics is treating the city as a decorated
plane. The city is a stack of surfaces and permissions. A roof is visible but
not always reachable. A wall is present until a door or breach changes it. A
window reveals without admitting movement. A market stall obscures without
becoming a fortress. A crowd blocks a decision even if it does not block a
path.

The engine should keep making those distinctions explicit. The more the native
rendering improves, the more important it becomes that every visible surface has
a truthful C-side interpretation.

## Movement Failure Evidence

Movement failure should be visible in more than one place:

- command return code;
- last error or debug text;
- route failure counter;
- replay event;
- unit or civilian snapshot field;
- optional overlay for blocked route.

The point is not to flood the UI. The point is to make pathing bugs cheap to
find. When an AI-only battle stalls, a route failure should leave enough
evidence to explain why.

## LOS Review Protocol

A new LOS rule should be reviewed against a small set of map situations:

- clear street to clear street;
- street to interior through a door;
- street to interior through a window;
- interior to interior through a wall;
- rooftop to street;
- rooftop across roof edge;
- blocked alley through rubble;
- civilian shelter from outside fire lane.

These examples should become tests or repeatable debug scenarios over time.
