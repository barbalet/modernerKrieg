# Chapter 6: Civilians As Agents

The Mosul demo cannot treat civilians as scenery. Civilians must move, freeze,
flee, shelter, follow instructions, become wounded, become evacuated, and shape
the outcome of the battle. They are part of the simulation's moral and tactical
surface.

The core already exposes civilian state and intent. The scenario data includes
civilian archetypes, civilian groups, and civilian spawn zones. The runtime art
includes civilian standing, wounded, and dead states across facings. This is a
foundation worth protecting.

![Civilian runtime example](../assets/shared/runtime/sprites/rendered/civilians_128/civilian/old_man/standing/north.png)

## Why Civilian AI Belongs In C

Civilian movement cannot be left to the Mac frontend. If it were, AI-vs-AI
tests would not see it. Replay would not reproduce it. CI failures would not
catch it. A Windows build might behave differently from a Mac build. The score
could drift depending on presentation timing.

Civilian AI should live in the deterministic core or in a C policy layer that
emits normal core intent updates. It should use the same topology and risk
queries as armed AI. It should be visible in snapshots and replay.

## Civilian State

The current state model includes:

- sheltering;
- fleeing;
- frozen;
- following instructions;
- evacuated;
- wounded;
- dead.

This is enough to describe the first public demo if the transitions are clear.
A civilian can begin in a market crowd, freeze under nearby fire, flee along a
safe graph route, follow a player instruction, reach a shelter, or evacuate.
A wounded civilian should change local risk and urgency. A dead civilian should
affect score, after-action text, and the emotional reading of the replay.

## Civilian Intent

Intent is separate from state:

- shelter;
- flee;
- evacuate;
- follow instructions;
- freeze;
- assist group.

This distinction matters. A civilian may intend to flee but be blocked. A
civilian may intend to follow instructions but panic after a shot. A civilian
may intend to assist a group member but be unable to reach them. Intent lets
the engine explain what the civilian is trying to do, not just where they are.

## Stress And Risk

Civilian behavior should respond to:

- nearby gunfire;
- visible casualties;
- armed movement close by;
- active fire lanes;
- blocked paths;
- dangerous objectives;
- crowd density;
- group separation;
- shelter or exit availability;
- player instructions.

Stress should not be random color. It should be an accumulated pressure that
changes decisions. Low stress civilians may shelter or comply. High stress
civilians may flee, freeze, or make unsafe crossings. Wounded civilians should
change the stress of nearby group members.

The engine already tracks civilian risk. The next strong version should make
risk local, inspectable, and route-aware. "Civilian risk 7" is useful. "This
civilian is crossing a restricted fire lane from `market_stalls_ground` to
`south_exit` under recent suppressive fire" is better.

## Groups

Civilian groups are important because people rarely behave as independent
particles in a crisis. A family, market crowd, shop residents, or shelter group
should have cohesion. Group members may cluster, separate, assist each other,
or choose between leaving together and fleeing individually.

The first implementation can keep this simple:

- each civilian has a group id;
- each group has a preferred shelter or exit;
- panic can spread within range;
- wounded group members can pull others toward assistance;
- group separation can raise stress.

This gives the AI enough social texture without making the simulation too
expensive.

## Civilian Routes

Civilian movement should be graph-based, not straight-line panic through walls.
The topology manifest already names shelters, exits, shops, streets, alleys,
and doors. Civilian pathing should use those records and expose route failures.

The civilian route cadence can be slower than unit movement. Civilians do not
need to reconsider every tick. A lower cadence makes behavior cheaper and more
legible. The important thing is that when they do reconsider, the choice is
deterministic from seed, state, stress, and local risk.

## Visuals And Respect

The current runtime civilian sprites include standing, wounded, and dead
states. Those images should be handled carefully. They exist to make the
simulation accountable, not to sensationalize harm.

![Civilian source state sheet](../assets/shared/source/sprite_sheets/19_civilian_states_topdown_128.png)

The renderer should make civilians visible enough that the player cannot
pretend they are absent. The UI should also avoid turning them into visual
noise. Civilian state, risk, and intent should be readable through selection,
overlays, and after-action text.

## The Esoteric Civilian Model

Combat games often model courage for soldiers and ignore fear for everyone
else. Mosul asks for something harder. A civilian's fear is not a special
effect. It changes streets, timing, lines of fire, and the moral shape of an
otherwise tactical decision.

The engine does not need melodrama. It needs consistent consequences. If a
patrol fires through a crowded market, the state should remember. If the patrol
opens a safer route, civilians should be able to use it. If an AI rushes an
objective while civilians are exposed, the score and replay should say so.

## Civilian AI Test Cases

Civilian behavior should be tested through small deterministic scenarios:

- civilians remain sheltered when no threat appears;
- civilians freeze under nearby fire;
- civilians flee away from a dangerous fire lane;
- civilians follow instructions when stress is low enough;
- civilians fail to route when all exits are blocked;
- civilians choose a safer longer route over a short exposed route;
- a wounded civilian changes group behavior;
- evacuation improves after-action evidence.

These tests should not require the native frontend. They should run through
the same C fixed-step path as AI-only battles.

## Civilian Rendering Questions

The renderer should be able to answer:

- which civilians are visible?
- which are selected or inspected?
- which state sprite should be drawn?
- which route or intent overlay should be shown?
- which civilians are in risk zones?
- which have evacuated and should no longer be drawn on-map?

If the draw command vocabulary cannot answer those questions, the native UI
will be tempted to infer civilian state. That inference should be avoided.
