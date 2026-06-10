# Runtime Maps

Generated map overviews, tiles, masks, and navigation products belong here.

Current runtime product:

- `market_commercial_streets_2003/overview.png`: copied from the source `preview_1400.png` for the first PNG-backed demo pass. Future asset tooling may regenerate this from layers instead of copying the preview.
- `market_commercial_streets_2003/levels/`: four 7,000 px line-art floor PNGs copied from the approved source stack. Level 1 is the ground map. Level 2 includes second-floor art and roofs for one-storey structures. Levels 3 and 4 are alpha overlays and remain empty wherever no higher floor or roof-access art has been authored.

Vehicle-free rerender candidates:

- `market_commercial_streets_2003/candidates/vehicle_free_candidate_1254.png`: generated line-art candidate for removing baked vehicles from the base map.
- `market_commercial_streets_2003/candidates/vehicle_free_candidate_first_pass_1254.png`: earlier generated pass retained for comparison.

The candidate images are not live gameplay map layers. They are lower
resolution review artifacts and are not pixel-aligned with the current
building-level JSON, topology graph, LOS blockers, movement blockers, or
upper-floor overlays. Do not replace `overview.png` or `levels/level_01_ground.png`
with these candidates without regenerating the full 7,000 px aligned map stack
or the matching gameplay metadata.
