/*
 * Temporary diagnostics for the Britain stable horse.
 *
 * This header is intentionally present only on
 * exult-horse-collision-diagnostics. It does not alter pathfinding,
 * collision flags, scheduling or movement.
 */
#ifndef EXULT_HORSE_COLLISION_DIAGNOSTICS_H
#define EXULT_HORSE_COLLISION_DIAGNOSTICS_H

#include "actors.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>

inline bool ExultHorseTraceEnabled(const Actor* actor) {
    if (!actor || actor->get_shapenum() != 727)
        return false;
    const Tile_coord pos = actor->get_tile();
    return pos.tx >= 1060 && pos.tx <= 1110 &&
           pos.ty >= 2140 && pos.ty <= 2200;
}

inline void ExultHorseTrace(const char* format, ...) {
    // Reopen per record so a crash will not lose a buffered trace.
    // The file is created in Exult's process working directory.
    std::FILE* file = std::fopen("exult-horse.log", "a");
    if (!file)
        return;

    using Clock = std::chrono::steady_clock;
    static const auto start = Clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - start).count();
    std::fprintf(file, "EXULT HORSE ms=%lld ", static_cast<long long>(elapsed));
    va_list args;
    va_start(args, format);
    std::vfprintf(file, format, args);
    va_end(args);
    std::fputc('\n', file);
    std::fclose(file);
}

#endif  // EXULT_HORSE_COLLISION_DIAGNOSTICS_H
