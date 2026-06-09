# Chapter 7: Hidden Information And Search

Urban tactical play depends on imperfect knowledge. A contact may be real,
suspected, false, concealed, revealed, searched, or misunderstood. A cache may
exist behind a door. A rooftop may be empty or dangerous. A civilian movement
may be mistaken for a threat. The engine needs to model this uncertainty
without becoming nondeterministic.

The public contact report kinds include:

- fire;
- reveal;
- civilian risk;
- suspected danger;
- false contact;
- search;
- breach.

This is a good starting vocabulary. It lets the engine report what happened
and what might be happening.

![Urban tactics line art](../assets/mosul/source/line_art/07_tactics_urban_combat.png)

## Hidden Contacts

Hidden contacts should not be ordinary visible units with alpha set to zero.
They are knowledge states. A hidden opfor element may be physically present in
the core, but the player side may only have a suspected contact report. AI
should reason from what the side knows, not from omniscient state.

For AI-only battles, this distinction still matters. A player-side tactical AI
should investigate, overwatch, withdraw, or suppress based on reports available
to that side. An opfor AI can use its own knowledge. Replay should preserve
when knowledge changed.

## Suspected And False Contacts

Suspected and false contacts are especially useful in Mosul. Dense buildings,
roof edges, market clutter, and civilians create ambiguous signals. The engine
can represent uncertainty by generating reports with confidence, terrain
metadata, and source context.

False contacts should not simply be bugs. They are part of the model when they
arise from plausible uncertainty. The important thing is that they remain
traceable. A false contact that cannot be explained in debug text will feel
like random punishment.

## Search

Search is the bridge between uncertainty and knowledge. Search outcomes include:

- clear;
- cache found;
- threat revealed;
- civilian found;
- booby trap;
- intelligence.

Search should depend on topology and role. A unit searching a cache semantic
zone should not be the same as a unit glancing at open street. Engineers,
leaders, training, stress, time pressure, and nearby danger should eventually
affect search quality.

Search should also have memory. A searched portal or semantic zone should be
marked. The demo audit already tracks searched portals and searched semantic
zones. That state should flow into replay and snapshots so the UI can show what
has been cleared and what has not.

## Breach

Breach outcomes include none, already open, breached, and unsafe. A breach is
not only a shortcut. It changes topology state. It may create a new movement
path, a new line of sight, a new danger, or a new civilian route.

The engine should treat breach as a portal-state change, not a visual effect.
The renderer can draw an overlay. The rules should decide whether a portal is
open, unsafe, searched, or still blocked.

## Contact Reports As Evidence

Contact reports are valuable because they are both gameplay and debugging
evidence. A headless transcript that says `contacts=2` is helpful, but a richer
report can say what kind of contact, where it came from, which side knows it,
which topology node it belongs to, and whether it was resolved.

The replay validator should keep these reports stable. If a future change makes
contacts appear earlier, later, or with different confidence, that may be a
real design change. It should be visible.

## AI And Uncertainty

AI that sees everything is easier to write and less useful. The current tactical
AI should continue moving toward knowledge-limited behavior:

- investigate suspected or false contact reports;
- overwatch likely threat routes;
- avoid civilian-risk fire lanes;
- withdraw or rally under suppression;
- search cache zones and interiors;
- breach only when it helps the mission or safety.

The opfor AI also needs uncertainty. It can know its own hidden units, but it
should still respond to patrol movement, search pressure, civilian flow, and
line-of-sight exposure.

## The Esoteric Side Of Fog

Fog of war is not simply a black overlay. It is the difference between the
world as it is and the world as a side can act upon. In Mosul, that gap is the
game. A doorway is not only closed or open; it is unknown. A roof is not only
high; it is watched or unwatched. A civilian is not only a sprite; they can
change what a soldier thinks is safe to do.

The engine should make uncertainty legible without making it magical. Every
report should come from a cause. Every reveal should have a reason. Every false
contact should be explainable as a failure of knowledge, not a failure of code.

## Contact Report Lifecycle

A strong contact report lifecycle should include:

- creation source;
- owning side;
- map position;
- topology node or semantic zone when known;
- confidence;
- age;
- resolution state;
- related unit, civilian, portal, or search id when applicable.

Reports should expire, resolve, or be superseded deliberately. Otherwise the
map becomes cluttered with stale uncertainty and the AI starts chasing ghosts in
ways that are hard to debug.

## Search As A Conversation With The Map

Search should ask the topology what is available. A search in an open street,
shop interior, cache zone, rooftop, shelter, or blocked building should not be
identical. The search action should be a conversation between unit capability,
stress, time, local threat, and semantic zone.

That gives the player and AI reasons to choose who searches and when. It also
gives the after-action report more to say than "objective checked."
