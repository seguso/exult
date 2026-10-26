# Modern camera picking: attempted double compensation (2026-10-26)

**Status: REVERTED. Exult devmix's existing implementation is correct; do not reapply the October 26 mouse adapter.**

## Symptoms and root cause

A proposed fix copied from C# Ult into Exult's `gamewin.h` and replaced four `gamewin.cc` calls. User's actual gameplay showed a serious regression: clicking a visible chest in an upper-floor room selected nearby closed shutters, and picking was broken across floors.

The assumption behind the proposed fix was false. In **`Modern_view_coordinates.inc`**, included by `gamewin.cc`, Exult **already** compensates the smooth-camera delta in `Game_window::display_to_world`:

```cpp
const int visual_x = static_cast<int>(std::lround(smooth_cam_x));
const int logical_x = scrolltx * c_tilesize + scrolltx_lo;
x += wrapped_delta(visual_x, logical_x);
```

`Game_window::world_to_display` already applies the symmetric inverse **before** rotating the view. This integration was overlooked because its implementations live in an included `.inc` file rather than directly in `gamewin.cc`. The added `display_to_rendered_world` applied another smooth-camera correction after `display_to_world`; the same correction was performed twice.

## Reversion

Restore the exact prior `gamewin.h` and `gamewin.cc` blob contents from `5437920ec3b5832dcd101eaf2c5f95d3304cb38d`, retain the existing `Modern_view_coordinates.inc`, and remove the incorrect earlier fix documentation.

Incorrect commits (historical only): `9f6c2040154eac9072d167eb4386145a8f86e237`, `f30388392e66bb4b7b4a1bd42ad78e73c1ba1193`, `b5dfdfedc1bd7b6b1b55b7bfc94570fe9c909aa1`, `1fd47e10f1902f02bd0e239a617ddbfb39a673ec`.

Rollback: `07a14d37477d44dd3ff975a56c5a90bf68934cc7`, `04477d0771c409d899ef244247f4661a8d0d87f4`, `a0d547e27227645e9097cf72e257e8a655b16f13`.

**Validation:** Both restored file blob SHAs verified equal to the original parent commit. Exult build/runtime after rollback not executed here; user will retest. Do not describe the mistaken patch as an Exult bug fix.

## Correct action in Ult

The C# `GameWindow.DisplayToWorld` and `WorldToDisplay` implementations had omitted this exact part of the C++ source. Ult's previous `DisplayToRenderedWorld` adapter repaired one direction at some call sites but was not a faithful direct port. Port both canonical coordinate methods from `Modern_view_coordinates.inc`, let callers invoke the canonical methods, and avoid any additional compensation.
