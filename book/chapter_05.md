# Chapter 5: Units, Soldiers, And Equipment

The game is unit based, but the unit is not the end of the model. A patrol
element is commanded as a unit because that is how tactical decisions stay
readable. Inside the unit, the engine needs soldiers. A leader, rifleman,
automatic rifleman, breacher, medic, marksman, RPG gunner, or machine gunner
does not contribute the same things to a fight.

This is one of the main ways `modernerKrieg` differs from
`derZweiteWeltkrieg`. The older engine can reason with tabletop-style units and
profiles. The Mosul demo needs unit orders with soldier-level consequences.

![Combatant top-down source sheet](../assets/shared/source/sprite_sheets/08_combatants_topdown_128.png)

## Unit Command, Soldier Consequence

The core exposes orders at the unit level:

- hold;
- move;
- assault move;
- fire;
- suppress;
- overwatch;
- breach;
- rally;
- withdraw;
- investigate.

The soldier model carries details that affect the unit's ability to carry out
those orders. A unit with wounded soldiers may still move, but slowly. A unit
with a suppressed automatic rifleman may have less suppressive capacity. A unit
with a breacher near a portal may have different options. A unit with high
stress may become pinned or broken.

This preserves readability. The player does not need to drag every soldier
individually, but the result of the player's choices can still land on
individual people.

## Roles

The current public role enum includes:

- rifleman;
- leader;
- machine gunner;
- RPG;
- marksman;
- engineer;
- medic;
- drone operator;
- civilian.

That list is a contract with future content. A role should be more than a
label. It should eventually influence weapon access, AI behavior, search
quality, breach capability, casualty impact, morale, and visual representation.

The current sprite manifest already gives the renderer a way to distinguish
roles visually. The C engine should keep building toward role-specific
capability without making the UI infer behavior from an image.

![U.S. ally force archetypes](../assets/mosul/source/line_art/10_us_ally_force_archetypes.png)

## Stance And Wounds

Stance currently includes standing, crouching, and prone. Wound state runs from
none to killed. Those two axes are enough to make a runtime sprite set feel
alive and enough to make simulation state more explicit.

Runtime infantry sprites cover 16 combatant roles, 5 body states, and 8
facings. That is 640 infantry PNGs. The engine should use those assets through
manifest ids, not through hard-coded renderer guesses.

For the simulation, stance should influence exposure, movement, observation,
fire, and suppression. Wounds should influence movement, morale, evacuation,
medical priority, and scoring. The first implementation can be simpler than
the final ambition, but the data model is pointed in the right direction.

## Weapons

Weapons are not only icons. They are constraints on range, fire mode, ammo,
suppression, penetration, casualty risk, and civilian-risk pressure.

![Infantry and support weapons line art](../assets/mosul/source/line_art/05_weapons_infantry_support.png)

The runtime weapon set contains 64 weapon sprites: 8 weapon types by 8 facings.
That is enough to let the renderer show dropped weapons, carried weapons,
inspector details, or action previews without inventing art ids.

The C model should treat weapons as equipment attached to soldiers or vehicles.
The renderer can show the asset. The rules should decide whether the weapon can
fire, suppress, breach, or influence a contact report.

## Vehicles And Traffic

The runtime vehicle set includes combat/support vehicle states and dynamic
traffic vehicles. Traffic vehicles are especially important for Mosul because a
city street should not be an empty arena. Cars, pickups, buses, and motorcycles
can block sight, constrain movement, shelter civilians, or become hazards.

![Vehicles and air systems line art](../assets/mosul/source/line_art/06_vehicles_and_air_systems.png)

The core already has traffic vehicle kinds and boarding modes. The board-view
layer can collect traffic vehicle markers, and the demo draw commands include
traffic vehicles. This is the correct early shape. Vehicles become visible
engine objects, not baked background clutter.

## Suppression And Morale State

The unit status enum gives the model four immediate states:

- ready;
- suppressed;
- pinned;
- broken.

The difference matters. Suppressed units can still exist as tactical objects,
but their fire, movement, and decision quality should suffer. Pinned units may
need rally, withdrawal, or cover. Broken units should stop behaving like
ordinary combat elements.

Stress connects soldier-level experience to unit-level status. This is where
the model becomes more than hit points. A unit can be physically intact but
mentally unable to carry out the plan. In an urban setting, that is not an
ornament. It is one of the central effects of hidden threats and fire lanes.

## Visual Consistency

The current sprite assets are top-down alpha PNGs. They need transparent edges,
consistent states, and stable facing names. The engine should not care how a
PNG is encoded, but it should validate that manifests point to a complete,
reviewable runtime set.

The book can use the source and runtime images as plates because they reveal
whether the model has enough visual vocabulary. If the engine has prone,
wounded, dead, crouching, and standing states, the artwork should make those
states legible. If the artwork has states the engine ignores, that is also a
design prompt.

## The Unit As A Small Society

The esoteric part of the unit model is that a squad is not a blob. It is a
small social and tactical organism. A leader changes confidence. A casualty
changes tempo. A breacher changes the meaning of a doorway. A medic changes
the moral weight of time. A machine gunner changes what suppression means.

`modernerKrieg` does not need to simulate every private thought of every
soldier to honor that. It does need enough soldier-level state that unit
outcomes feel earned rather than arbitrary.

## Soldier State Review Questions

When adding or changing soldier fields, ask:

- does the field affect an order?
- does it affect movement, LOS, cover, fire, morale, search, breach, or aid?
- should the AI know it?
- should it appear in a snapshot?
- should it influence sprite selection?
- should it be recorded in replay or after-action output?

If the field affects only presentation, it may belong near draw commands. If it
affects consequences, it belongs in the core model.

## Equipment And Visual Drift

Equipment is a place where art and simulation can drift quickly. A sprite sheet
may show a weapon before the engine knows its range. The engine may model a
breaching tool before the art has a clear silhouette. The answer is not to
block either side. The answer is to keep manifests honest and to document the
current gap.

The book should keep using plates to expose those gaps. A missing capability is
easier to discuss when the visual vocabulary is on the page.
