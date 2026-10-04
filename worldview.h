/*
 *  Centralized transform between the logical world view and the displayed
 *  view.  The world itself remains in its normal Exult coordinate system.
 */

#ifndef INCL_WORLDVIEW
#define INCL_WORLDVIEW

#include <algorithm>
#include <array>
#include <cmath>

struct World_view_point {
	double x = 0;
	double y = 0;
};

struct World_view_rect {
	double x = 0;
	double y = 0;
	double w = 0;
	double h = 0;
};

class World_view_transform {
	bool enabled = false;
	int display_width = 0;
	int display_height = 0;
	int scene_size = 0;
	int scene_offset_x = 0;
	int scene_offset_y = 0;

	static constexpr double half_sqrt_2 = 0.7071067811865475244;

	World_view_point rotate(World_view_point p, double angle) const {
		const double cx = display_width * 0.5;
		const double cy = display_height * 0.5;
		const double x  = p.x - cx;
		const double y  = p.y - cy;
		return {cx + x * std::cos(angle) - y * std::sin(angle),
				cy + x * std::sin(angle) + y * std::cos(angle)};
	}

public:
	World_view_transform() = default;

	World_view_transform(int width, int height, bool on = false) {
		configure(width, height);
		enabled = on;
	}

	void configure(int width, int height) {
		display_width  = std::max(1, width);
		display_height = std::max(1, height);
		// The bounding square of a W x H rectangle rotated by 45 degrees.
		scene_size = static_cast<int>(std::ceil((display_width + display_height) * half_sqrt_2)) + 2;
		scene_size = std::max(scene_size, std::max(display_width, display_height));
		scene_offset_x = (scene_size - display_width) / 2;
		scene_offset_y = (scene_size - display_height) / 2;
	}

	bool is_enabled() const {
		return enabled;
	}

	void set_enabled(bool on) {
		enabled = on;
	}

	int get_display_width() const {
		return display_width;
	}

	int get_display_height() const {
		return display_height;
	}

	int get_scene_size() const {
		return scene_size;
	}

	int get_scene_offset_x() const {
		return scene_offset_x;
	}

	int get_scene_offset_y() const {
		return scene_offset_y;
	}

	World_view_point scene_to_display(World_view_point p) const {
		return enabled ? rotate(p, 3.14159265358979323846 * 0.25) : p;
	}

	World_view_point display_to_scene(World_view_point p) const {
		return enabled ? rotate(p, -3.14159265358979323846 * 0.25) : p;
	}

	World_view_rect transform_rect(const World_view_rect& r) const {
		if (!enabled) {
			return r;
		}
		const std::array<World_view_point, 4> corners = {{{r.x, r.y}, {r.x + r.w, r.y},
				{r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}}};
		World_view_point p = scene_to_display(corners[0]);
		World_view_rect out{p.x, p.y, 0, 0};
		double right  = p.x;
		double bottom = p.y;
		for (size_t i = 1; i < corners.size(); ++i) {
			p = scene_to_display(corners[i]);
			out.x = std::min(out.x, p.x);
			out.y = std::min(out.y, p.y);
			right = std::max(right, p.x);
			bottom = std::max(bottom, p.y);
		}
		out.w = right - out.x;
		out.h = bottom - out.y;
		return out;
	}

	static bool self_test() {
		World_view_transform t(320, 200, true);
		const World_view_point center{160, 100};
		const World_view_point c2 = t.display_to_scene(t.scene_to_display(center));
		if (std::abs(c2.x - center.x) > 1e-9 || std::abs(c2.y - center.y) > 1e-9) {
			return false;
		}
		for (const World_view_point p : {World_view_point{160, 60}, World_view_point{200, 100},
				World_view_point{160, 140}, World_view_point{120, 100}}) {
			const World_view_point round_trip = t.display_to_scene(t.scene_to_display(p));
			if (std::abs(round_trip.x - p.x) > 1e-9 || std::abs(round_trip.y - p.y) > 1e-9) {
				return false;
			}
		}
		const World_view_rect box{140, 80, 40, 40};
		const World_view_rect rotated = t.transform_rect(box);
		if (rotated.w < 56.5 || rotated.w > 57.0 || rotated.h < 56.5 || rotated.h > 57.0) {
			return false;
		}
		World_view_transform identity(320, 200, false);
		const World_view_point p{17, 23};
		const World_view_point q = identity.scene_to_display(p);
		return q.x == p.x && q.y == p.y && identity.display_to_scene(p).x == p.x && identity.display_to_scene(p).y == p.y;
	}
};

#endif
