# Build Matrix

The skeleton should remain buildable without SDL3. SDL3 enables the windowed app, but the portable core, tests, and headless runner must continue to work on their own.

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

Use the default preset when SDL3 is available or when checking that the app target skips cleanly:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

On Apple Silicon with Homebrew SDL3, prefer the arm64 default preset:

```sh
cmake --preset default-arm64
cmake --build --preset default-arm64
ctest --preset default-arm64
```

The SDL app has a deterministic smoke mode:

```sh
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/default-arm64/bin/modernerKrieg --project-root . --ai-only --smoke-frames 2
```

Create the current macOS-first smoke-tested package:

```sh
cmake --build --preset default-arm64 --target modernerKrieg_macos_smoke_package
```

The package target copies the SDL binary, scenario file, manifests, and the referenced PNG assets into `build/default-arm64/package/modernerKrieg-macos-smoke/`, smoke-tests that copied layout with dummy video, and writes `build/default-arm64/package/modernerKrieg-macos-smoke.zip`.

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

- macOS: install CMake and optionally SDL3 through Homebrew. The headless preset should work without SDL3. On Apple Silicon, Homebrew SDL libraries are arm64, so use `default-arm64` for the SDL-enabled CMake app build. The `modernerKriegAIBattles.xcodeproj` project builds a command-line AI battle runner directly in Xcode without SDL.
- Windows: use a C compiler supported by CMake, such as MSVC. SDL3 should provide `SDL3Config.cmake` or be passed through `SDL3_DIR`.
- Linux: install CMake, a C compiler, and optionally SDL3 development packages that provide the CMake config file.

## CI Smoke Checks

A basic CI job should run:

```sh
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
./build/strict/bin/mk_headless_run --steps 3 --quiet
./build/strict/bin/mk_ai_battle --battles 1 --ticks 5 --quiet
./build/strict/bin/mk_ai_battle --battles 5 --ticks 160 --seed 84985359904819 --seed-step 101 --quiet --fail-on-stall --expect-settled 5 --expect-max-stalled 0 --expect-min-worst-score 440
./build/strict/bin/mk_replay_validate --help
```

AI-vs-AI checks should keep adding seed-controlled autoplay runs, balance expectations, and replay playback validations, not just process exit codes.
