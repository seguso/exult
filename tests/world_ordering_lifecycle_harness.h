/*
 * Diagnostic-only adapter for deterministic world-ordering fixtures.
 * This file is kept in the Exult checkout, not copied into Ult.
 */
#ifndef WORLD_ORDERING_LIFECYCLE_HARNESS_H
#define WORLD_ORDERING_LIFECYCLE_HARNESS_H

#include <iosfwd>
#include <string>

class Game_window;
class Map_chunk;
class Game_object;

class World_ordering_lifecycle_harness {
public:
	static int run(Game_window* window, const std::string& fixture_path, const std::string& output_path);

private:
	struct State;
	static State* active_state;
	static void on_comparison(Map_chunk* chunk, Game_object* newobj, Game_object* existing, int raw, int final);
	static void on_paint_chunk(int cx, int cy);
	static void on_paint_object(Game_object* obj);
	static std::string id_for(Game_object* object);
	static void write_snapshot(
			std::ostream& output, const std::string& fixture_name, int step, const std::string& operation, const std::string& id);
};

#endif
