/*
 * Copyright (C) 2026 The Exult Team
 * GPL-2.0-or-later
 *
 * Visual-only heading for Modern Mouse Target.  This samples a smoothed
 * version of the ACTUAL A* route, never a straight line to its destination.
 * There are no changes to the walkable tiles or to animation step timing.
 */
#ifndef EXULT_MODERN_MOUSE_HEADING_H
#define EXULT_MODERN_MOUSE_HEADING_H

#include <algorithm>
#include <cmath>
#include <vector>

class Modern_mouse_heading {
public:
    struct Point {
        double x;
        double y;
    };

    // Exult has only four walking-art directions (0, 2, 4, 6).
    int direction() const { return facing; }

    void reset() {
        facing = -1;
        pending = -1;
        pending_steps = 0;
        smooth_x = smooth_y = 0.0;
        initialized = false;
    }

    // Called only when an actual A* walking step is about to be performed.
    // Future points start with the tile about to be entered and continue
    // along the unconsumed path (up to eight tiles).
    int advance(Point current, const std::vector<Point>& future, int world_size) {
        if (future.empty()) return facing;
        std::vector<Point> route;
        route.reserve(future.size() + 1);
        route.push_back(current);
        for (const Point& next : future) {
            const Point& prev = route.back();
            // The U7 map wraps; unwrap adjacent steps into a local plane.
            auto delta = [world_size](double n, double p) {
                double d = n - p;
                if (world_size > 0) {
                    const double half = world_size * 0.5;
                    if (d > half) d -= world_size;
                    if (d < -half) d += world_size;
                }
                return d;
            };
            const Point p{prev.x + delta(next.x, prev.x), prev.y + delta(next.y, prev.y)};
            if (std::hypot(p.x - prev.x, p.y - prev.y) > 0.01) route.push_back(p);
        }
        if (route.size() < 2) return facing;

        // Two rounds of Chaikin corner cutting: smooth the route for VISUAL
        // tangent estimation only. It must never be used for actual motion
        // (which would cut corners through walls).
        for (int pass = 0; pass < 2 && route.size() < 100; ++pass) {
            std::vector<Point> finer;
            finer.reserve(route.size() * 2);
            finer.push_back(route.front());
            for (size_t i = 1; i < route.size(); ++i) {
                const Point a = route[i - 1], b = route[i];
                finer.push_back({0.75 * a.x + 0.25 * b.x, 0.75 * a.y + 0.25 * b.y});
                finer.push_back({0.25 * a.x + 0.75 * b.x, 0.25 * a.y + 0.75 * b.y});
            }
            finer.push_back(route.back());
            route.swap(finer);
        }

        const auto along = [&](double distance) {
            Point p = route.front();
            for (size_t i = 1; i < route.size(); ++i) {
                const double dx = route[i].x - p.x, dy = route[i].y - p.y;
                const double length = std::hypot(dx, dy);
                if (length >= distance && length > 0.0001)
                    return Point{p.x + dx * distance / length, p.y + dy * distance / length};
                distance -= length;
                p = route[i];
            }
            return route.back();
        };
        const Point near_point = along(1.0);
        const Point far_point = along(2.2);
        double dx = (near_point.x - current.x) * 0.70 + (far_point.x - current.x) * 0.30;
        double dy = (near_point.y - current.y) * 0.70 + (far_point.y - current.y) * 0.30;
        const double length = std::hypot(dx, dy);
        if (length < 0.001) return facing;
        dx /= length;
        dy /= length;

        // This state belongs to the walking action, NOT to the A* instance.
        // A new path every 500 ms retains the previous smoothed heading.
        if (!initialized) {
            smooth_x = dx;
            smooth_y = dy;
            initialized = true;
        } else {
            const double blend = 0.48;
            smooth_x = (1.0 - blend) * smooth_x + blend * dx;
            smooth_y = (1.0 - blend) * smooth_y + blend * dy;
            const double s = std::hypot(smooth_x, smooth_y);
            if (s > 0.0001) { smooth_x /= s; smooth_y /= s; }
        }

        const Point dirs[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
        const auto score = [&](int dir) {
            const Point d = dirs[dir / 2];
            return smooth_x * d.x + smooth_y * d.y;
        };
        int best = 0;
        for (int d = 2; d <= 6; d += 2)
            if (score(d) > score(best)) best = d;
        if (facing < 0) {
            facing = best;
            return facing;
        }
        if (best == facing) {
            pending = -1;
            pending_steps = 0;
            return facing;
        }

        // Hysteresis prevents switching at every NE/NW tile correction.
        // A genuine strong turn changes promptly; borderline changes must
        // persist through two walking steps. Retarget events aren't steps.
        const double advantage = score(best) - score(facing);
        if (advantage < 0.18) {
            pending = -1;
            pending_steps = 0;
            return facing;
        }
        if (best == pending) ++pending_steps;
        else { pending = best; pending_steps = 1; }
        if (advantage > 0.72 || pending_steps >= 2) {
            facing = best;
            pending = -1;
            pending_steps = 0;
        }
        return facing;
    }

private:
    int facing = -1;
    int pending = -1;
    int pending_steps = 0;
    double smooth_x = 0, smooth_y = 0;
    bool initialized = false;
};

#endif
