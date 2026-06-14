# Third-Party and Reference Code

`modernerKrieg` is intended to be a clean-break engine.

`derZweiteWeltkrieg` may be used as a design reference for architecture, deterministic tactical tests, thin presentation layering, and comparable rule structure. It is not a submodule, package dependency, or runtime dependency of this repository.

## Current Status

No third-party source code has been copied into this repository for the current skeleton.

## If Code Is Ported

Before copying or porting any code from another repository:

- confirm the source license allows the intended use
- record the source repository, commit, file path, and license in this document
- rename and adapt the code into the `modernerKrieg` domain
- add tests that prove the behavior in this engine
- keep copied code out of `engine/core` unless it is portable C and belongs below platform/content layers

## Attribution Log

| Source | Commit | Files | License | Notes |
| --- | --- | --- | --- | --- |
| None yet | N/A | N/A | N/A | The current code is original project skeleton work. |
