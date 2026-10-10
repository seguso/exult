# Exult feature separation plan (base 0e3cd67bc751223b24e41d4bc6ef9aebd8830ac4)

This branch is an **analysis/coordination branch**, not yet an executable shared-code prerequisite. It intentionally makes no runtime changes. Do not stack feature branches on it until the actual shared infrastructure is extracted and tested.

## Preliminary feature areas

- `feature/camera`: modern smooth camera, velocity/tau, optional smooth avatar walk.
- `feature/mouse-target`: independent A* mouse target, right-click/hold/release, mouse arrow behavior.
- `feature/rotation`: optional 45-degree world view, sampling quality, screen/world transform and associated render/drag/input corrections.
- `feature/fonts`: optional readable conversation font, font file/family selection, sizing/tracking, glyph generation and text paint.
- `feature/crt`: optional CRT filter, settings and renderer integration.
- `feature/input`: independent WASD/diagonal input options (needs distinct review from mouse target).
- `feature/ui-options` (possible shared infrastructure): child settings dialogs, consistent disabled-control visual state, and reusable option widget behavior.

## Findings from comparing base to devmix

The full branch is 186 commits ahead of the base; the history is *not* feature-atomic. Multiple features edit the same files, especially `gamewin.cc/.h`, `exult.cc`, `gamerend.cc`, and `gumps/GameDisplayOptions_gump.cc/.h`. Shared file edits are not sufficient evidence of a required feature dependency.

The main potential reusable foundation is settings UI infrastructure (child dialogs and disabled-control opacity/hit-test behavior). It is currently integrated with feature-specific code in `GameDisplayOptions_gump.cc`; before placing it in a common code commit it must be isolated, built, and tested against this base alone. Rendering and coordinate conversion must be evaluated separately: rotation-specific transforms should not be pulled into unrelated features by accident.

Mouse target **must remain independent of camera**. Camera on/off must preserve mouse-target click/hold/release semantics. Smooth avatar walk currently depends on modern camera rendering and can be reviewed as a separate dependent patch.

## Proposed extraction and validation procedure

1. Keep `devmix` as integration/testing branch. Do not rewrite its history.
2. Exclude local/development/reference artifacts from upstream: `.vscode/`, `verify2-*.txt`, `_reference/`, and personal launch/config files unless explicitly justified.
3. Isolate the minimal common UI code (if any) into a separately reviewable commit, **without** including a feature. Build and test it against the upstream base first.
4. Create feature branches from the upstream base (or that *verified* common prerequisite only where truly needed). Transplant each feature by curated hunks/commits, not blindly cherry-picking 186 development commits.
5. For each branch: compile relevant platforms/configurations, test feature disabled by default and feature enabled, and verify no accidental changes to original Exult options.
6. Test representative feature combinations on an integration branch and compare behavior to `devmix`.
7. Submit small upstream PRs; explicitly document a dependency only when one PR genuinely requires another. Rebase as upstream accepts changes.

## Current status

- [x] Verified comparison base -> devmix and identified major feature areas.
- [x] Created `feature/common` from the requested base.
- [ ] Isolate and validate actual reusable runtime/UI infrastructure.
- [ ] Create self-contained, buildable feature branches.
- [ ] Validate patch independence and combinations.

This file tracks the analysis; it is **not** a claim that the common runtime extraction or individual feature branches are ready.
