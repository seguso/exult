/*
 * Copyright (C) 2026 The Exult Team
 * GPL-2.0-or-later
 *
 * Visual-only facing from the local smoothed A* route for Modern Mouse Target.
 * The movement path and walking animation phase remain untouched.
 */
#ifndef EXULT_MODERN_MOUSE_HEADING_H
#define EXULT_MODERN_MOUSE_HEADING_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

class Modern_mouse_heading {
public:
    struct Point {
        double x;
        double y;
    };

    int direction() const { return facing; }

    void reset() { walked.clear(); facing = -1; }

    // Only actual traversed tiles enter the history, not planned A* points.
    void remember(Point current, int world_size) {
        if (!walked.empty()) {
            const Point last = walked.back();
            const double dx = wrapped_delta(current.x, last.x, world_size);
            const double dy = wrapped_delta(current.y, last.y, world_size);
            const double d = std::hypot(dx, dy);
            if (d < 0.01) return;
            // A teleport/new unrelated movement must not join the old curve.
            if (d > 1.6) walked.clear();
        }
        walked.push_back(current);
        // Only the recent past affects a local Chaikin tangent; bound work
        // during long walks while keeping history across every A* replan.
        constexpr std::size_t max_walked = 24;
        if (walked.size() > max_walked)
            walked.erase(walked.begin(), walked.begin() + (walked.size() - max_walked));
    }

    // Computes the tangent at the CURRENT position. No temporal filtering,
    // no hysteresis, and no modification to movement or animation indices.
    // future[0] is the next tile to be walked into, not the final target.
    int calculate(Point current, const std::vector<Point>& future, int world_size) const {
        if (future.empty() && walked.empty()) return facing;
        struct Sample { Point p; double index; };
        std::vector<Sample> curve;
        curve.reserve(walked.size() + future.size() + 1);
        auto append = [&](Point raw) {
            if (curve.empty()) {
                curve.push_back({raw, 0.0});
                return;
            }
            const Point prev = curve.back().p;
            const double dx = wrapped_delta(raw.x, prev.x, world_size);
            const double dy = wrapped_delta(raw.y, prev.y, world_size);
            if (std::hypot(dx, dy) > 0.01)
                curve.push_back({{prev.x + dx, prev.y + dy}, static_cast<double>(curve.size())});
        };
        for (const Point& p : walked) append(p);
        append(current);
        if (curve.empty()) return facing;
        const double current_index = curve.back().index;
        for (const Point& p : future) append(p);
        if (curve.size() < 2) return facing;

        // The attached index tracks the current waypoint through smoothing,
        // so a U-shaped path cannot accidentally project onto the wrong leg.
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<Sample> refined;
            refined.reserve(2 * curve.size());
            refined.push_back(curve.front());
            for (std::size_t i = 1; i < curve.size(); ++i) {
                const Sample& a = curve[i - 1];
                const Sample& b = curve[i];
                const auto mix = [](const Sample& a, const Sample& b, double t) -> Sample {
                    return {{a.p.x + (b.p.x - a.p.x) * t,
                             a.p.y + (b.p.y - a.p.y) * t},
                            a.index + (b.index - a.index) * t};
                };
                refined.push_back(mix(a, b, 0.25));
                refined.push_back(mix(a, b, 0.75));
            }
            refined.push_back(curve.back());
            curve.swap(refined);
        }

        std::vector<double> distance(curve.size(), 0.0);
        for (std::size_t i = 1; i < curve.size(); ++i)
            distance[i] = distance[i - 1] + std::hypot(
                curve[i].p.x - curve[i - 1].p.x,
                curve[i].p.y - curve[i - 1].p.y);

        // Arc-length coordinate of the current waypoint on the smoothed curve.
        double at = distance.back();
        for (std::size_t i = 1; i < curve.size(); ++i) {
            if (curve[i].index >= current_index) {
                const double dt = curve[i].index - curve[i - 1].index;
                const double t = dt > 0.0 ? std::clamp(
                    (current_index - curve[i - 1].index) / dt, 0.0, 1.0) : 0.0;
                at = distance[i - 1] + t * (distance[i] - distance[i - 1]);
                break;
            }
        }
        const auto sample = [&](double arc) -> Point {
            arc = std::clamp(arc, 0.0, distance.back());
            auto it = std::lower_bound(distance.begin(), distance.end(), arc);
            if (it == distance.begin()) return curve.front().p;
            if (it == distance.end()) return curve.back().p;
            const std::size_t i = static_cast<std::size_t>(it - distance.begin());
            const double length = distance[i] - distance[i - 1];
            if (length <= 0.0001) return curve[i].p;
            const double t = (arc - distance[i - 1]) / length;
            return {curve[i - 1].p.x + t * (curve[i].p.x - curve[i - 1].p.x),
                    curve[i - 1].p.y + t * (curve[i].p.y - curve[i - 1].p.y)};
        };

        // Symmetric tangent: include the incoming and outgoing geometry,
        // avoiding the directional lag caused by pure forward lookahead.
        constexpr double half_window = 1.8;
        const Point before = sample(at - half_window);
        const Point after = sample(at + half_window);
        const double dx = after.x - before.x;
        const double dy = after.y - before.y;
        if (std::hypot(dx, dy) < 0.0001) return facing;
        // Four cardinal sprite directions: N=0, E=2, S=4, W=6.
        if (std::abs(dx) > std::abs(dy)) return dx > 0.0 ? 2 : 6;
        return dy > 0.0 ? 4 : 0;
    }

    void set_direction(int dir) { if (dir >= 0) facing = dir; }

private:
    static double wrapped_delta(double next, double prev, int world_size) {
        double diff = next - prev;
        if (world_size > 0) {
            const double half = world_size / 2.0;
            if (diff > half) diff -= world_size;
            else if (diff < -half) diff += world_size;
        }
        return diff;
    }

    std::vector<Point> walked;
    int facing = -1;
};

#endif
