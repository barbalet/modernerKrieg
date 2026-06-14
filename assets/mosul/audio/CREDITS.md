# MosulGame Audio Credits

The current MosulGame soundscape ships with an original procedural audio set
generated for the project. These assets are intentionally restrained: they
provide tactical UI feedback, low-level ambient texture, sparse radio cues, and
non-lexical civilian murmur without using third-party recordings or archival
material.

## Original Procedural Assets

Attribution: MosulGame procedural audio generator, 2026-06-14

License category in manifest: `Original`

Files:

- `cues/ui_order_arm.wav`
- `cues/ui_order_confirm.wav`
- `cues/ui_invalid.wav`
- `cues/tactical_tick.wav`
- `cues/tactical_movement.wav`
- `cues/tactical_contact.wav`
- `cues/tactical_route_blocked.wav`
- `cues/tactical_fire.wav`
- `cues/tactical_objective.wav`
- `cues/tactical_risk.wav`
- `loops/ambient_city_low.wav`
- `loops/ambient_city_high.wav`
- `loops/ambient_generator.wav`
- `loops/ambient_engine_distant.wav`
- `loops/civilian_murmur_low.wav`
- `loops/civilian_murmur_high.wav`
- `voices/radio_move_set.wav`
- `voices/radio_contact_reported.wav`
- `voices/radio_no_line_of_sight.wav`
- `voices/radio_route_blocked.wav`
- `voices/radio_civilians_close.wav`
- `voices/radio_task_complete.wav`
- `voices/radio_hold_position.wav`

Generation source: `scripts/generate_mosul_audio_assets.py`

The civilian murmur loops are non-lexical and marked with locale `zxx`; they do
not contain Arabic words or simulated phrases. The radio voice assets are
procedural radio-tone surrogates with English transcripts and visible captions
in the manifest. Future recorded Iraqi speech or U.S. radio performances must
receive separate speaker review, transcript review, and license approval before
they replace these generated placeholders.

Release audio may use only original work, commissioned work, CC0, CC BY with
attribution, or audited public-domain U.S. Government material. Noncommercial,
unsourced, ripped, or unclear-license material must not be added to the runtime
payload.
