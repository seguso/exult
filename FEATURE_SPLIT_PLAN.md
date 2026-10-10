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


## Upstream hygiene and instrumentation audit (2026-10-10)

**Hard exclusion rule for all curated feature branches:** Do not transplant personal `.cfg` files, launch configurations, workstation paths, ad-hoc scripts, log output files, temporary reference copies, or debugging/diagnostic console/file logging introduced during feature development. Do not strip existing upstream diagnostic/error reporting: distinguish new instrumentation from upstream behavior by comparing against base `0e3cd67bc751223b24e41d4bc6ef9aebd8830ac4`.

**Concrete non-upstream artifacts found in `base..devmix` (exclude by default):**
- `.vscode/exult-launch.cfg`, `.vscode/launch.json`, `.vscode/tasks.json`
- `baseline-exult.cfg`
- `verify2-stdout.txt`, `verify2-stderr.txt`
- `_reference/exult-rotate/exult-1.5-rotate-v0.3.patch` and `_reference/exult-rotate/exult-1.5-rotate-v0.3-1.patch`
- `Avvia-Ultima6.bat` (personal launcher; exclude unless separately justified)

**Identified instrumentation requiring removal during extraction:**
- `gamerend.cc`: one-time `std::cout` instrumentation beginning `Forward quadruplets ALGO=WEIGHTED-QUADRUPLETS-V1`, `Forward pairs ALGO=WEIGHTED-PAIRS-V1`, and `Forward triplets ALGO=WEIGHTED-TRIPLETS-V1`; remove counters and caches too if they serve only those logs.
- `gumps/GameDisplayOptions_gump.cc`: `[FONT-PICKER]` acceptance/cancel/error and installed-font chooser logs, plus `Readable conversation font selected:` console print. Keep actual user-facing error handling/behavior, not the console trace.
- Review other touched files for newly introduced `std::cout`, `std::cerr`, `printf`, `fprintf`, `ofstream`, `fopen`, temporary `DEBUG` traces, and emitted log files. **Do not automatically delete** upstream error handling, player-facing diagnostics, or code that is actually part of a supported product feature.

**Transplant gate (required for every feature branch):** (1) list changed paths vs upstream; (2) scan added diff lines for debug/logging and personal paths; (3) verify no forbidden paths in branch diff; (4) compile + functional verification; (5) compare settings UX, especially disabled controls, to integration branch. Git clean/conflict-free alone does not satisfy this gate.

The six feature branches currently point to the clean upstream base, so their diffs contain none of these artifacts. `devmix` is preserved untouched as a reference.


## First implementation boundary review (2026-10-10)

Comparison against the original upstream file tree found that `gumps/InputOptions_gump.cc` grows from 289 to 311 lines and `gumps/InputOptions_gump.h` from 116 to 126 in `devmix`. Both input features touch this menu; their presentation must be independently transplantable even when the other feature is omitted.

- WASD/diagonal keyboard movement: keep keyboard dispatch/options and the true input-state behavior in `feature/input`. Do not incorporate A* mouse target or camera state into the keyboard patch.
- Mouse target: `feature/mouse-target` owns its path retargeting/right-click timing and arrow behavior. Its Game Input toggle must be optional and cannot require `feature/input`.
- Menu co-location does not imply a runtime feature dependency. At shared call sites prefer a small isolated conditional integration hunk per feature over a generalized dispatcher that would be costly to upstream reviewers.
- UI helper is a shared *rendering mechanism* only: it cannot be the authoritative source for whether a control is enabled. Inactive controls must also be excluded from hit testing, including mouse hold/release paths.

**Observed change sizes (devmix vs upstream baseline):** `InputOptions_gump.cc` 289 -> 311 lines; `InputOptions_gump.h` 116 -> 126 lines; `keyactions.cc` 987 -> 1025 lines; `keys.cc` 775 -> 777 lines. Line counts alone do not establish ownership; inspect hunks when transplanting.

**Implementation order:** validate the common helper; then start with the smaller WASD input feature, then mouse target, then independent video/font modules. Preserve `devmix` for parity/regression tests and do not copy debug logging or personal files.


## Shared Shift-toggled movement speed: verified dependency (2026-10-10)

**New discovery:** the speed mode is *shared runtime behavior*, not merely two independent reads of SDL Shift. This requires its own small common component. The previous feature boundary audit missed this dependency.

Direct source inspection of `devmix:exult.cc`:
- Lines ~229-244: `keyboard_medium_speed` defaults to false (fast); `modern_mouse_walk_speed()` reads it, and left/right Shift candidate flags track standalone key taps.
- Line ~1547: modern WASD passes `keyboard_medium_speed ? 1 : 0` into the keyboard walk actions.
- Lines ~1498 and ~2022: modern mouse A* speed uses `modern_mouse_walk_speed()`.
- Lines ~2271-2301: Shift keydown/up toggle detection is wrongly nested under `if (modern_keyboard)`. When the modern keyboard feature is disabled and mouse target is enabled, Shift cannot toggle the shared speed. **This is a real pre-existing devmix behavioral dependency/bug**, not just a patch split concern.
- The upstream base already contains normal `SDL_KMOD_SHIFT` handling and keybinding modifiers; do not replace or change those semantics when both modern features are off.

**Required common feature**: reusable standalone-Shift-tap speed state machine, with no WASD or A* references. Inputs: key-down/up scancode, repeat and intervening non-Shift key activity; state: fast/medium, left/right Shift tap candidates; output: whether speed toggled. Keep the speed popup/feedback separately optional so core shared state is not coupled to layers/UI and so both features may request it. Handle focus loss and/or feature deactivation without stale candidates; distinguish physical Shift hold from completed standalone taps. Do not accidentally intercept Shift+Q, Shift+W/A/S/D, or pre-existing upstream key bindings.

**Integration rule**: process the common speed toggle only when `modern_keyboard_enabled || modern_mouse_target_enabled`. Keyboard walking reads speed mode if enabled; mouse A* speed calculation reads it if enabled. Neither feature must test the other's enable flag. Preserve default fast and user-visible speed behavior from devmix whenever both are enabled.

**Acceptance matrix:** keyboard only (on/off speed), mouse target only (on/off speed), both enabled (same shared mode), neither (no added Shift semantics); standalone LShift/RShift, held Shift, Shift+letter, key repeat, loss of focus, toggling feature flags while pressed, and clicks during walking. Verify the mouse-only case explicitly (regression against devmix). Ensure no custom log messages and no personal cfg artifacts.

**Architecture**: one small header/source or self-contained helper for speed state, with a minimal event integration hunk in `exult.cc`. Feature-specific walking and A* algorithms stay on their own branches. This common module may justify a real dependency for these two branches, but not for CRT, fonts or rotation.

Current status: analysis complete, code extraction/integration not yet implemented or build-tested.

## Integration bug fix already published to devmix

- Commit `64c771f601b68e0b64aa4080043da94fc9f724c4` corrects `exult.cc` so Shift speed toggling is active when either modern keyboard **or** modern mouse target is enabled. This fixes mouse-only mode, without maintaining the old bug.
- This is the minimal integration hotfix; it is **not** yet the final common-module refactoring. `Modern_movement_speed.h` exists on `feature/common`, but event wiring, focus reset, and integration tests are still outstanding. Do not falsely claim this is completed merely because both branches contain related code.
- The final extraction should substitute the shared class for the local state and candidate flags in `devmix` and preserve mouse-only behavior. The feature branches must consume that shared class, not each other's code.


## Shift shared runtime integration tranche (2026-10-10)

Published on `devmix`:
- `ae216dea519ace0cf5292ea32c6ec2f5314fc276`: imports the identical `Modern_movement_speed.h` shared state machine from `feature/common`.
- `d4fe2199cc251ba10e6596575299782abfe96edc`: rewires modern keyboard and A* mouse paths to `modern_movement_speed.is_medium()`, replaces local Shift candidate flags with shared key-down/up processing, and cancels pending taps on focus loss or when neither consumer is enabled.
- Static comparison from `64c771f` to current `devmix`: exactly the new header and `exult.cc` changed, in two commits. Old speed state and candidate variables no longer occur.

**Important**: no build or live gameplay run has been completed. Validate Debug/Release compilation and the complete state matrix (WASD only, mouse target only, both, neither; standalone left/right Shift, modifiers, focus loss, repeat). The common header still requires explicit test coverage. Do not make the mouse feature depend on the keyboard feature.
