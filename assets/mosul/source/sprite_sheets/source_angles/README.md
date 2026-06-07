# Source Sprites

This directory contains the three hand-rendered or AI-rendered source angles used by the sprite pipeline.

Run:

```bash
.venv-imagegen/bin/python tools/render_assets.py scaffold
```

That command creates the expected directory tree and writes `missing_sources.jsonl`, one JSON object per missing source PNG. Each entry includes:

- `path`: where the approved source PNG should be saved.
- `kind`: `infantry`, `weapon`, or `vehicle`.
- `faction`: `allied`, `opposing`, or blank for weapon pickup art.
- `item_id`: unit, vehicle, or weapon id.
- `state`: body or damage state, when applicable.
- `angle`: one of `north`, `north_east`, or `east`.
- `prompt`: a concise generation prompt for contemporary black-and-white line art.

Do not store derived flips here. Only source angles belong in this tree.
