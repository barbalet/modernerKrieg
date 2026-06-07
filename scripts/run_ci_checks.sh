#!/usr/bin/env bash

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="${MK_CI_LOG_DIR:-$ROOT_DIR/build/ci-logs}"
TIMESTAMP="${MK_CI_TIMESTAMP:-$(date -u +"%Y%m%dT%H%M%SZ")}"
LOG_FILE="$LOG_DIR/modernerKrieg-ci-$TIMESTAMP.txt"

mkdir -p "$LOG_DIR"
printf "%s\n" "$LOG_FILE" > "$LOG_DIR/latest-log-path.txt"

run() {
  printf "\n+"
  printf " %q" "$@"
  printf "\n"
  "$@"
}

find_executable() {
  local build_dir="$1"
  local executable_name="$2"
  local executable_path

  executable_path="$(find "$ROOT_DIR/$build_dir" -type f \( -name "$executable_name" -o -name "$executable_name.exe" \) | sort | head -n 1 || true)"
  if [[ -z "$executable_path" ]]; then
    printf "Could not find executable %s under %s\n" "$executable_name" "$ROOT_DIR/$build_dir" >&2
    return 1
  fi

  printf "%s\n" "$executable_path"
}

run_whitespace_check() {
  if ! git diff --quiet || ! git diff --cached --quiet; then
    run git diff --check
    run git diff --cached --check
    return
  fi

  if git rev-parse --verify HEAD^ >/dev/null 2>&1; then
    run git diff --check HEAD^ HEAD
    return
  fi

  run git diff --check
}

run_asan_core_check() {
  if [[ "${MK_CI_RUN_ASAN:-1}" != "1" ]]; then
    return
  fi

  if [[ "${RUNNER_OS:-}" == "Windows" ]]; then
    printf "\nSkipping AddressSanitizer check on Windows runner.\n"
    return
  fi

  run cmake -S . -B build/asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMK_BUILD_TESTS=ON \
    "-DCMAKE_C_FLAGS=-fsanitize=address -fno-omit-frame-pointer" \
    "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"
  run cmake --build build/asan --target mk_core_tests --config Debug
  run "$ROOT_DIR/build/asan/bin/mk_core_tests"
}

ci_main() {
  local headless_run
  local ai_battle

  set -e
  cd "$ROOT_DIR"

  printf "modernerKrieg CI checks\n"
  printf "timestamp_utc=%s\n" "$TIMESTAMP"
  printf "runner_os=%s\n" "${RUNNER_OS:-local}"
  printf "root=%s\n" "$ROOT_DIR"
  printf "log_file=%s\n" "$LOG_FILE"

  run git rev-parse HEAD
  run cmake --version

  run cmake --preset default
  run cmake --build --preset default --config Debug
  run ctest --preset default --output-on-failure -C Debug

  run cmake --preset strict
  run cmake --build --preset strict --config Debug
  run ctest --preset strict --output-on-failure -C Debug

  headless_run="$(find_executable build/default mk_headless_run)"
  ai_battle="$(find_executable build/default mk_ai_battle)"

  run "$headless_run" --steps 1 --briefing
  run "$ai_battle" --battles 5 --ticks 160 --summary-every 40 --fail-on-stall --expect-max-stalled 0 --quiet

  run_asan_core_check
  run_whitespace_check
}

ci_main 2>&1 | tee "$LOG_FILE"
status=${PIPESTATUS[0]}

printf "\nCI log written to %s\n" "$LOG_FILE"
exit "$status"
