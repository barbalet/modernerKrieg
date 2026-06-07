# Build Matrix

The portable core, demo session API, tests, headless runner, AI battle runner, replay tools, and renderer-independent projection layer should remain buildable without any platform UI dependency. Native Mac and Windows frontends should sit above these targets.

## Local Presets

```sh
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/mk_headless_run --steps 10
```

Use the strict preset before larger changes:

```sh
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
```

Use the default preset for the ordinary portable C build:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The default and strict suites include `mk_demo_session_tests`, which exercises
the native-wrapper C contract without launching a platform frontend.

Run repeated AI-vs-AI battles from CMake:

```sh
cmake --build build --target mk_ai_battle
./build/bin/mk_ai_battle --battles 5 --ticks 160 --summary-every 10 --watchdog 40
```

Add `--keep-running-after-outcome --fail-on-stall` when you want a longer soak that keeps ticking after an objective outcome and returns nonzero on a tactical stall.

Run a deterministic seed-sweep balance check:

```sh
./build/bin/mk_ai_battle --battles 5 --ticks 160 --seed 84985359904819 --seed-step 101 --quiet --fail-on-stall --expect-settled 5 --expect-max-stalled 0 --expect-min-worst-score 440
```

Validate generated replay/event logs:

```sh
./build/bin/mk_headless_run --ai-only --max-ticks 3 --quiet --replay build/mosul_ai.mkreplay
./build/bin/mk_replay_validate --expect-result MK_OK build/mosul_ai.mkreplay
./build/bin/mk_replay_validate --playback --from-tick 2 --to-tick 3 build/mosul_ai.mkreplay
```

Run the same AI battle tool through the checked-in Xcode command-line project:

```sh
xcodebuild -project modernerKriegAIBattles.xcodeproj -scheme mk_ai_battle -configuration Debug build
```

The shared Xcode scheme launches with the five-seed balance arguments enabled, so running it in Xcode should fail visibly if the batch no longer settles cleanly, stalls, or drops below the expected worst-score floor.

## Platform Notes

- macOS: install CMake and use the checked-in `modernerKriegAIBattles.xcodeproj` project for the command-line AI battle runner. The native Mac frontend should link `modernerKriegDemo` and keep gameplay in C.
- Windows: use a C compiler supported by CMake, such as MSVC. The native Windows frontend should link `modernerKriegDemo` and keep gameplay in C.
- Linux: install CMake and a C compiler for headless/autoplay validation. A Linux presentation layer is not part of the current native frontend pivot.

## GitHub Actions Checks

The GitHub Actions workflow in `.github/workflows/c-engine-ci.yml` runs on
push, pull request, and manual dispatch. Per-commit verification is expected to
happen there, not as a required local pre-commit step.

The workflow writes a timestamped log under `build/ci-logs/`. Failed workflow
runs upload that text file as a downloadable artifact named with the runner OS
and UTC start time, so the log can be handed back for diagnosis. The workflow
runs default and strict CMake builds, default and strict CTest suites, headless
smoke output, an AI-vs-AI stall guard, an AddressSanitizer core test on
macOS/Linux, and whitespace checks against the checked commit.

AI-vs-AI checks should keep adding seed-controlled autoplay runs, balance
expectations, and replay playback validations, not just process exit codes.
