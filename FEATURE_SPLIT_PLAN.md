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
- [x] Create six feature branch refs from upstream base (code not yet transplanted).
- [ ] Populate and build-test each feature branch.
- [ ] Validate patch independence and combinations.

This file tracks the analysis; it is **not** a claim that the common runtime extraction or individual feature branches are ready.


## Shared disabled-option rendering and interaction audit (2026-10-10)

**Decision: candidate for actual common infrastructure, independent of the feature-specific dependency rules.**

Inspection of `devmix:gumps/GameDisplayOptions_gump.cc` found:

- `is_dependent_option_inactive(button_ids)` computes feature-specific dependencies: original smooth scrolling when modern mode is active; smooth-avatar-walk and camera tau when modern mode is off; rotation sampling when rotation is off; CRT parameters when CRT is off; font source/size controls when readable fonts are off.
- `on_button(int,int)` skips inactive controls, independently of their visual presentation.
- `paint()` snapshots the indexed 8-bit framebuffer underneath each disabled row, paints labels and buttons normally, then alpha-blends the completed row at 50% via `Palette::find_color` and a 65,536-entry cache.
- Rows must not overlap (the implementation deliberately uses row pitch, rather than a larger fixed rectangle). Entire rows include both label and control; a separate font-summary row is handled specially.

**Extraction boundary**: move reusable 8-bit palette-aware blending/backdrop logic to a small general-purpose gump/UI helper. It should accept explicit bounded rectangles and an alpha parameter (default 50%), with no reference to camera/CRT/rotation/fonts and no direct dependence on `GameDisplayOptions_gump`. Feature-owned callers decide which controls are inactive; the existing gump/event mechanism prevents activation. Do not introduce global feature switches or rewrite original Exult controls. Keep paint ordering deterministic and do not alter layout or saved settings.

**Caution**: the current drawing path is a post-paint framebuffer effect, *not* real opacity on a widget. Copying it blindly into a generic `Gump_button::paint()` would fade the button without its label and could break palette/modal semantics. Prefer explicit backdrop capture before control-and-label paint and blend after paint; alternatively use a composited row helper if the existing UI architecture supports that cleanly.

**Validation gates before declaring common runtime commit complete**:

1. Build with only the upstream base plus the shared helper: no feature-specific references.
2. Test 50% blend on indexed 8-bit palette, clipping, overlapping rows, and palette selection; ensure enabled rows are unchanged.
3. Verify hit-test/activation suppression and state preservation in each eventual feature branch.
4. Confirm original upstream option layout, mouse behavior and settings are unchanged.
5. Keep `feature/common` analysis-only until this helper has been extracted and validated; no speculative dependency from every feature branch.

Current audit finding: isolated palette compositor header added as `gumps/Option_row_fader.h` (commit `5da48e8749afba0789d47f052d0ce607a20cd59d`). It is **not yet wired into any UI, compile-tested, or verified at runtime**. Feature-specific enablement/disablement is intentionally excluded. Next: add a minimal test/compile harness, then integrate it into one option dialog and verify byte-identical output against devmix for representative palettes.
