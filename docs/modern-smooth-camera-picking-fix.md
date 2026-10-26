# Modern smooth camera: world pointer hit testing (2026-10-26)

Branch: `seguso/exult@devmix`. Port of the viewport-input alignment verified to resolve elevated-story world object captions in `seguso/ult`.

## Root cause

`Modern_camera_render.inc::paint_current_view` and `paint_velocity_camera` temporarily install interpolated scroll coordinates for painting, then restore logical `scrolltx/scrollty`. The normal world `show_items` / `double_clicked` and related selection paths in `gamewin.cc` previously used `display_to_world` alone (undoes 45-degree transform), while `find_object` relies on the restored logical scroll. When smooth and logical positions differ, visible sprites and mouse tests do not refer to the same pixels. This issue is particularly noticeable for densely stacked upper-storey objects and underlying floor.

## Fix

- New `Game_window::display_to_rendered_world(int&,int&) const` in `gamewin.h`. First calls the canonical `display_to_world`, then subtracts the wrapped, pixel-rounded difference between logical and actually painted smooth camera origins. No effect when modern movement or the camera is inactive.
- Four ordinary world selection call sites in `gamewin.cc` now use the corrected conversion, including item name clicks, party selection, click-to-object and double clicks.
- No new hit-testing, per-object Z filtering, shape pixel rule, or sorting algorithm.
- `<cmath>` included explicitly for `std::fmod` and `std::lround`.

**Verification:** GitHub source update only. Not compiled, not run. Test manually on the first story with modern smooth camera and 45-degree rotation on, clicking doors, windows, books and furniture both moving and stationary; also check ground-floor and dragging behavior. If drag starts still differ, trace `find_object_from_display` and drag.cc separately, without bypassing the canonical path.
