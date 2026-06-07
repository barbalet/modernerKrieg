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

Run repeated AI-vs-AI battles from CMake:

```sh
cmake --build build --target mk_ai_battle
./build/bin/mk_ai_battle --battles 5 --ticks 160 --summary-every 10 --watchdog 40
```

Run the same AI battle tool through the checked-in Xcode command-line project:

```sh
xcodebuild -project modernerKriegAIBattles.xcodeproj -scheme mk_ai_battle -configuration Debug build
```

## Platform Notes

- macOS: install CMake and optionally SDL3 through Homebrew. The headless preset should work without SDL3. The `modernerKriegAIBattles.xcodeproj` project builds a command-line AI battle runner directly in Xcode without SDL.
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
```

Future AI-vs-AI checks should add seed-controlled autoplay runs and compare event logs or replay snapshots, not just process exit codes.
