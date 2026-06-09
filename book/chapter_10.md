# Chapter 10: AI Battles, Replay, And Evidence

`modernerKrieg` has to be playable by people, but it also has to be playable by
itself. AI-only battle runs are not a side feature. They are how the project
finds stalls, broken assets, impossible routes, bad scoring, hidden-contact
bugs, and civilian logic problems before a native UI hides them behind a pretty
frame.

The important tools live in
[`../engine/tools/autoplay/`](../engine/tools/autoplay/):

- `mk_headless_run`;
- `mk_ai_battle`;
- `mk_replay_validate`.

## Headless Runs

`mk_headless_run` loads a scenario and steps the engine without a window. It
can run AI-only, override seed and tick counts, write transcripts, emit
after-action summaries, and generate replay files.

This should remain the fastest way to ask: does the engine still work? A
frontend bug may be annoying. A headless failure means the simulation itself
needs attention.

## AI-vs-AI Battles

`mk_ai_battle` runs repeated deterministic battles. It can report summaries,
watchdog stalls, best and worst scores, settlement expectations, and failure
conditions. This is the tool that emerged after early runs showed obvious
problems: units holding, opfor not contesting, scores decaying, and battles
ending without meaningful tactical behavior.

That history is useful. It says the project should trust batch evidence more
than a single manual run.

AI-vs-AI does not mean the game is designed only for AI. It means every system
has to operate when nobody is clicking. Routes must progress. Civilians must
move. Objectives must change hands. Search must resolve. Contacts must reveal
or remain uncertain for a reason. Scores must settle.

## Replay

Replay turns a battle into evidence. A replay file should record enough event
data that a future build can validate or play back a timeline. The validator
can reject malformed files and report specific tick ranges.

Replay should continue to capture:

- scenario identity;
- seed;
- orders;
- movement and route events;
- contact reports;
- civilian state changes;
- search and breach events;
- objective control;
- scoring and outcome;
- after-action summary.

As the engine grows, replay should become more valuable, not less. A native Mac
implementation can eventually use replay to scrub through an AI battle and
render a visual explanation of what happened.

## CI Logs

The GitHub Actions workflow under
[`../.github/workflows/c-engine-ci.yml`](../.github/workflows/c-engine-ci.yml)
runs the C engine checks on commit. The associated script
[`../scripts/run_ci_checks.sh`](../scripts/run_ci_checks.sh) captures useful
output. When a run fails, the downloadable log becomes part of the development
conversation.

This is a practical loop:

1. CI fails with a timestamped log.
2. The log is downloaded.
3. The failure is reproduced or reasoned from deterministic output.
4. The engine is fixed.
5. Tests and AI-only runs prove the fix.

That loop is especially important for data-heavy work. A missing PNG, broken
topology id, or changed score threshold can be as serious as a C bug.

## Xcode Command-Line Project

The repository includes `modernerKriegAIBattles.xcodeproj`. It exists to run
C AI battles from Xcode, giving immediate insight into engine behavior without
waiting for the future Mac UI.

This is a useful compromise. The project can use Xcode as a familiar Mac tool
while still testing the same portable C engine. The scheme should remain a
command-line simulation, not a second implementation of rules.

## What AI Should Prove

AI-only battles should prove more than "the program exits." They should catch:

- no side moves;
- routes fail repeatedly;
- objectives never become contested;
- civilians never update;
- hidden contacts never resolve;
- search orders never happen;
- scoring drifts endlessly downward;
- outcome thresholds are unreachable;
- replay output cannot validate;
- asset references are missing;
- topology debug reports warnings.

The tests do not need to prove the game is perfectly balanced. They need to
prove that the battle is alive and the systems can interact.

## What AI Should Not Do

AI should not use private actions. It should issue ordinary core orders. It
should not see hidden state unavailable to its side unless the scenario
explicitly gives it that knowledge. It should not rescue a broken scenario with
hard-coded coordinates when topology ids exist.

The player-side AI and opfor AI can use different policies, but they should
share the same engine truths. If AI has a special path through the code, it
will stop being a test of the actual game.

## Evidence Over Impression

The esoteric strength of deterministic replay is humility. A human playtest can
feel convincing and still miss a broken edge. A log can be dull and reveal the
truth. A seed can be replayed. A score can be compared. A route failure can be
counted. A contact report can be inspected. A civilian panic can be traced.

`modernerKrieg` is aiming at a rich and atmospheric demo, but the atmosphere
should sit on evidence. Headless AI battles are how the project keeps its feet
on the ground.

## Useful Failure Artifacts

When CI or a local run fails, the best artifact is not only the exit code. A
useful failure bundle should include:

- scenario id and name;
- seed and seed step;
- tick limit;
- command-line arguments;
- transcript or replay path;
- final score and outcome;
- route failure counts;
- civilian risk and casualty counts;
- contact report summary;
- topology audit summary;
- after-action text.

The current workflow already moves in this direction. Keep the failure artifact
human-readable so it can be pasted back into development without extra tools.

## Balance And Correctness

AI battle expectations are partly balance checks and partly correctness checks.
A bad score can mean the AI is weak. It can also mean an objective cannot be
reached, a civilian route is broken, a hidden contact never reveals, or a
threshold no longer matches the scenario.

When an expectation fails, resist the urge to tune the number first. Read the
transcript. Ask which system stopped behaving.
