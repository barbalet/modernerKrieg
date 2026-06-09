# Chapter 9: Combat, Risk, And Restraint

Combat in `modernerKrieg` should be understood as consequence, not animation.
A shot is not only a flash. It may reveal a hidden unit, suppress a target,
wound a soldier, endanger a civilian, create a contact report, change morale,
alter a route, affect scoring, and rewrite what the AI believes.

The engine's job is to preserve those consequences in deterministic state.

![Weapons line art](../assets/mosul/source/line_art/11_us_ally_weapons_lineart.png)

## Fire And Suppression

The current order vocabulary includes fire, suppress, and overwatch. That
separation is important. Direct fire tries to damage a known target.
Suppressive fire tries to change what a target can safely do. Overwatch holds
the unit ready for opportunity.

In a Mosul street, suppression is often more important than immediate
casualties. A unit under fire may stop, crawl, withdraw, rally, or fail to
complete a search. A civilian near a fire lane may freeze or flee. A hidden
contact may be revealed by firing, but not necessarily destroyed.

Suppression should connect to soldier stress, unit status, and AI choices.
The renderer should show it, but the core should decide it.

## Casualties

Casualties need to land at the right level. A unit can lose strength, but the
engine should know which soldiers are wounded, incapacitated, or killed. This
allows role loss, morale effects, after-action summaries, and sprite selection
to remain coherent.

The current runtime sprites include wounded and dead states. Those states are
not the model by themselves. They are the visual vocabulary for model state.

![Runtime infantry example](../assets/mosul/runtime/sprites/rendered/infantry_128/allied/us_army_rifleman/standing/north.png)

## Civilian Risk

Civilian risk is not just a penalty after the fact. It should be a live
constraint on good play and good AI. The core already tracks civilian risk and
contact reports can include civilian-risk events. Scoring weights turn risk
into outcome pressure.

Risk should come from understandable sources:

- firing through or near civilian areas;
- moving armed units close to civilians;
- forcing civilians across fire lanes;
- leaving wounded civilians exposed;
- breaching or searching in occupied buildings;
- using suppressive fire near market crowds;
- failing to open safe routes.

If risk is explicit, the AI can avoid it and the player can learn from it. If
risk is hidden, the score feels arbitrary.

## Search, Breach, And Violence

Search and breach are combat-adjacent actions. They often happen under threat,
and they change the map. A breach can create movement and LOS. A search can
reveal a cache, threat, civilian, trap, or intelligence.

The restraint model should make these actions tactical rather than decorative.
For example:

- breaching a door near civilians may increase panic;
- searching under suppression may be slower or less reliable;
- clearing a cache can improve score or reduce future danger;
- opening a route can help civilians evacuate;
- unsafe breach outcomes should change AI willingness.

## Rooftops And Elevation

Rooftops matter because they change sight, exposure, and threat geometry. The
demo has roof access level art and topology. A rooftop threat scenario already
exists. The combat model should keep elevation as a real factor.

Elevation can affect:

- line of sight over low blockers;
- exposure to return fire;
- overwatch value;
- civilian visibility;
- route cost;
- objective control;
- search priority.

The key is to avoid treating a roof as simply another terrain color. It is a
different tactical surface.

## Weapons And State

The runtime weapon sprites include M16, M4, M249, M203, AK-pattern rifle, and
other weapon types. The source line art gives the book and future UI a reference
plate. The engine should continue moving toward equipment-state clarity:

- weapon id;
- ammo kind;
- fire mode;
- cooldown;
- reload state;
- role compatibility;
- suppression effect;
- civilian-risk profile.

Weapon visuals should never be the source of rule truth. But when the renderer
shows a weapon, it should be because the engine knows the weapon exists.

## After-Action Meaning

The scenario includes after-action text for success, partial success, and
failure. That text should become richer over time. A good after-action report
should explain why the result happened:

- objective control;
- civilian risk;
- civilian casualties;
- player casualties;
- time pressure;
- unresolved contacts;
- search/cache outcomes;
- route failures;
- evacuation success.

This is not only player feedback. It is test evidence. When AI-only battles
fail in CI, the after-action text can point to the system that drifted.

## The Esoteric Combat Model

Many tactics games ask "who won the exchange?" Mosul also asks "what did the
exchange do to the street?" Did civilians run? Did a hidden shooter reveal
their position? Did a patrol become pinned outside a doorway? Did a route close?
Did an objective become technically secured but morally compromised?

That is where the simulation earns its subject. Combat is not separate from
movement, civilians, topology, graphics, AI, or scoring. It is the pressure
that ties them together.

## Combat Debug Evidence

A combat event should be explainable after the fact. The engine should preserve
or expose:

- attacker id and side;
- target id and side;
- weapon or fire mode;
- LOS result and blocker if relevant;
- cover hint;
- range;
- suppression result;
- casualty or wound result;
- civilian-risk side effects;
- contact reports generated;
- score changes.

Not every frontend screen needs all of this at once. Logs, replay, tests, and
debug panels do.

## Restraint As A Rule Surface

Restraint should become a rule surface, not a moral lecture. The engine can
model it through risk, line of fire, civilian proximity, order choice, weapon
choice, route choice, and after-action scoring. That lets the player learn
through cause and effect.

A restrained AI is also easier to trust. It should not simply refuse to fire.
It should choose better positions, wait for clearer LOS, suppress away from
civilians, open routes, or investigate before escalating.
