# modernerKrieg Book

This directory is a source-oriented book for `modernerKrieg`, modeled after the
developer-book style used by ApeSDK and `derZweiteWeltkrieg`.

It explains the current shape of the engine as a portable C simulation for the
MOSUL public demo: the Mosul 2003 Market / Commercial Streets scenario,
multi-level map data, topology, units, soldiers, civilians, hidden contacts,
AI-only battle runs, replay, native-wrapper contracts, and the graphics asset
pipeline that gives the simulation its visual and atmospheric identity.

Start with [the synopsis](synopsis.md), then read the chapters in order.

## Contents

- [Synopsis](synopsis.md)
- [Chapter 1: The Shape Of The Demo Engine](chapter_01.md)
- [Chapter 2: The C Contract](chapter_02.md)
- [Chapter 3: Mosul As Data](chapter_03.md)
- [Chapter 4: The Multi-Level City](chapter_04.md)
- [Chapter 5: Units, Soldiers, And Equipment](chapter_05.md)
- [Chapter 6: Civilians As Agents](chapter_06.md)
- [Chapter 7: Hidden Information And Search](chapter_07.md)
- [Chapter 8: Movement, LOS, And Urban Surfaces](chapter_08.md)
- [Chapter 9: Combat, Risk, And Restraint](chapter_09.md)
- [Chapter 10: AI Battles, Replay, And Evidence](chapter_10.md)
- [Chapter 11: Graphics As Atmosphere And Interface](chapter_11.md)
- [Chapter 12: Extending The Demo](chapter_12.md)
- [Art Plate Index](art_plate_index.md)

## Fast Source Map

- Project overview: [`../README.md`](../README.md)
- Development ledger: [`../PLAN.md`](../PLAN.md)
- Engine architecture: [`../docs/engine_architecture.md`](../docs/engine_architecture.md)
- Asset pipeline: [`../docs/asset_pipeline.md`](../docs/asset_pipeline.md)
- Scenario format: [`../docs/scenario_format.md`](../docs/scenario_format.md)
- Core C API: [`../engine/core/include/mk_core.h`](../engine/core/include/mk_core.h)
- Core implementation: [`../engine/core/src/mk_core.c`](../engine/core/src/mk_core.c)
- Native wrapper API: [`../engine/demo/include/mk_demo.h`](../engine/demo/include/mk_demo.h)
- Asset manifest API: [`../engine/assets/include/mk_asset_manifest.h`](../engine/assets/include/mk_asset_manifest.h)
- Renderer-independent board API: [`../engine/render/include/mk_board_view.h`](../engine/render/include/mk_board_view.h)
- AI policy API: [`../engine/ai/include/mk_ai.h`](../engine/ai/include/mk_ai.h)
- Mosul scenario loader: [`../game/mosul/src/mk_mosul_demo.c`](../game/mosul/src/mk_mosul_demo.c)
- Demo scenario: [`../game/mosul/scenarios/market_commercial_streets_2003.mkscenario`](../game/mosul/scenarios/market_commercial_streets_2003.mkscenario)
- Asset review catalogue: [`../assets/ASSETS.md`](../assets/ASSETS.md)

## Art Plate Policy

The book uses committed Mosul assets as working plates. They are not decoration
alone. The map, building levels, source sheets, runtime sprites, and line art
are part of how the repository explains scale, concealment, civilians, vertical
movement, force identity, and the mood of a dense urban tactical model.

When the art is replaced or unified, update the plates here at the same time as
the manifests and `assets/ASSETS.md`. The book should remain a readable bridge
between the C engine and the game the native Mac and Windows shells will show.

## Maintenance Rule

When a file mentioned by this book moves, update the book in the same change.
When a scenario concept becomes real C state, add it to the relevant chapter.
When an art asset is replaced, keep the old prose honest rather than leaving a
ghost of a previous visual direction. The book is intentionally close to the
source tree, so stale links and stale claims are defects.
