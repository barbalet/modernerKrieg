# Chapter 11: Graphics As Atmosphere And Interface

The graphics in `modernerKrieg` have an unusual role. They are not only
presentation. They are documentation, atmosphere, interface, review surface,
and asset contract. The C engine does not render through a platform graphics
API, but it does validate and expose the assets that the native frontends will
draw.

This makes the book a natural place to include art plates. The images help a
reader understand the map, scale, actors, weapons, vehicles, civilians, and
urban combat vocabulary before opening the C files.

## Source Art And Runtime Art

The asset pipeline separates:

- `assets/mosul/source/`, unmodified source art and references;
- `assets/mosul/manifests/`, validated metadata;
- `assets/mosul/runtime/`, generated or copied runtime products;
- `assets/mosul/maps/`, playable map products;
- `assets/mosul/atlases/`, future packed atlas products;
- `assets/mosul/sprites/`, engine-ready extracted sprites.

This separation should stay. Source art can be replaced or rerendered. Runtime
art can be regenerated. Manifests explain how the engine understands both.

## Map Plates

The runtime overview gives the reader and future frontend a first visual
contract:

![Market overview](../assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png)

The level stack gives the simulation its vertical vocabulary. Even when the
native renderer later uses tiling, caching, mipmaps, or Metal textures, the
meaning of the levels should remain tied to the manifest.

## Line Art Plates

The source line art is useful as atmospheric documentation. It shows the visual
language of Mosul before final unified art replaces or refines it.

![Combatant types](../assets/mosul/source/line_art/04_combatant_types.png)

![Urban combat tactics](../assets/mosul/source/line_art/07_tactics_urban_combat.png)

These plates should not overpromise exact final art. Their job is to give the
book and maintainers a shared visual memory while the engine is still becoming
fully playable.

## Sprite Sheets

The source sprite sheets show categories at a glance:

![U.S. troops source sheet](../assets/mosul/source/sprite_sheets/12_us_ally_troops_topdown_128.png)

![Vehicles source sheet](../assets/mosul/source/sprite_sheets/14_us_ally_vehicles_topdown_128.png)

![Weapons source sheet](../assets/mosul/source/sprite_sheets/16_us_ally_weapons_topdown_128.png)

They are especially useful for design review. If a role exists in C but cannot
be recognized in art, the mismatch is visible. If art exists for a state the
engine ignores, that mismatch is also visible.

## Runtime Render Manifest

The compact C sprite manifest is
[`../assets/mosul/manifests/mosul_2003_sprites.spritemanifest`](../assets/mosul/manifests/mosul_2003_sprites.spritemanifest).
The full runtime render manifest is
[`../assets/mosul/runtime/sprites/rendered/render_manifest.json`](../assets/mosul/runtime/sprites/rendered/render_manifest.json).

The current runtime sprite set contains 1,088 PNGs:

- 640 infantry sprites;
- 168 civilian sprites;
- 64 weapon sprites;
- 216 vehicle sprites.

The C asset layer validates that render-manifest paths exist. This makes the
graphics part of CI, not a manual afterthought.

## Alpha And Edges

The runtime sprites should be alpha PNGs with fully transparent edges down to
where the figure or object begins. This matters because top-down sprites will
be composited over a detailed map. Bad edges turn into visual halos, selection
ambiguity, and poor screenshots.

The engine does not inspect every pixel during ordinary play, but the pipeline
and validation tools should keep enforcing the contract. A native renderer
should be able to trust that runtime sprites are ready to composite.

## Draw Commands

The C demo API exports draw commands. A command can name a kind, stable id,
label, asset path, side, order, map position, target position, screen position,
screen radius, facing, intensity, and selected state.

This is the right balance:

- C decides what exists and where it is.
- The native frontend decides how it looks on the platform.
- Assets provide the visual vocabulary.
- The book records the intended relationship.

Draw commands let the Mac implementation become elegant without becoming
authoritative about gameplay.

## Screenshots As Future Evidence

The project has discussed screenshot output as another testing surface. If the
renderer is in the native app, screenshots may live in Mosul. If the C engine
eventually emits renderable image products, screenshots may also live here.

Either way, the idea is sound: capture interesting battle states with
timestamped filenames in an ignored local directory. Screenshots of civilian
panic, rooftop overwatch, route failure, search, breach, or objective control
can become visual regression evidence.

The key is to keep screenshots tied to deterministic seed, tick, and scenario.
A pretty image without provenance is only a picture. A picture tied to replay
is evidence.

## The Esoteric Role Of Art

Art changes what the player believes the simulation can know. A line-art map
with interiors invites the player to ask whether interiors matter. A civilian
sprite makes civilian danger immediate. A roof plate makes vertical tactics
visible. A weapon sprite makes equipment specific.

That creates responsibility. If the graphics suggest detail the engine cannot
honor, the player will feel the gap. If the engine has detail the graphics
cannot show, the player will miss the meaning. The Mosul demo is strongest
when art and simulation meet at the same level of truth.

## Plate Index

The companion [Art Plate Index](art_plate_index.md) gathers the major images
used by this book and lists review questions for each group. Use it when the
art is being refreshed or when a native renderer needs to confirm which visual
families are expected.

## Renderer Implementation Notes

A native renderer should treat C draw commands as a scene description. It can
batch, cache, sort, animate, fade, highlight, or decorate those commands, but
the command list should remain the starting truth.

Practical renderer questions:

- does every draw command kind have a visible presentation?
- are alpha PNG edges clean against the map?
- do unit and civilian facings match movement direction?
- are large vehicle sprites scaled consistently with the map?
- are overlays readable without hiding civilians?
- can the user distinguish suspected, false, and revealed contacts?
- can the user see which level or roof surface is active?

Those questions belong in visual QA as much as code review.
