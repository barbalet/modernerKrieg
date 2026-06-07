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
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/default-arm64/bin/modernerKrieg --smoke-frames 2
```

Run repeated AI-vs-AI battles from CMake:

```sh
cmake --build build --target mk_ai_battle
./build/bin/mk_ai_battle --battles 5 --ticks 160 --summary-every 10 --watchdog 40
```

Add `--keep-running-after-outcome --fail-on-stall` when you want a longer soak that keeps ticking after an objective outcome and returns nonzero on a tactical stall.

Validate generated replay/event logs:

```sh
./build/bin/mk_headless_run --ai-only --max-ticks 3 --quiet --replay build/mosul_ai.mkreplay
./build/bin/mk_replay_validate --expect-result MK_OK build/mosul_ai.mkreplay
```

Run the same AI battle tool through the checked-in Xcode command-line project:

```sh
xcodebuild -project modernerKriegAIBattles.xcodeproj -scheme mk_ai_battle -configuration Debug build
```

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
./build/strict/bin/mk_replay_validate --help
```

AI-vs-AI checks should keep adding seed-controlled autoplay runs and replay validations, not just process exit codes.
