/*
 * Deterministic diagnostic adapter that drives Exult's real Map_chunk,
 * Game_object and Game_render lifecycle. No ordering decisions are recreated
 * here: comparisons, edge mutations, cleanup, chunk traversal and painting
 * are performed by the original engine methods.
 */
#include "world_ordering_lifecycle_harness.h"

#include "../game.h"
#include "../gamewin.h"
#include "../gamerend.h"
#include "../gamemap.h"
#include "../objs/chunks.h"
#include "../objs/objiter.h"
#include "../objs/objs.h"
#include "../objs/ordinfo.h"
#include "../shapeid.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if __has_include("../gitinfo.h")
#	include "../gitinfo.h"
#endif

#ifndef GIT_REVISION
#	define GIT_REVISION "0a9b64a6c4f02aafb552fe98d591e9e7d96c8815"
#endif

namespace {

constexpr const char* kExultSourceCommit = "0a9b64a6c4f02aafb552fe98d591e9e7d96c8815";

struct Fixture_object {
	std::string id;
	int         shape{};
	int         frame{};
	int         tile_x{};
	int         tile_y{};
	int         lift{};
	int         x_tiles{};
	int         y_tiles{};
	int         height{};
	bool        occludes{};
};

struct Fixture_operation {
	std::string name;
	std::string id;
	int         tile_x{};
	int         tile_y{};
	int         lift{};
	std::string other_id;
};

struct Fixture_case {
	std::string                  name;
	std::string                  game;
	int                          base_tile_x{};
	int                          base_tile_y{};
	std::vector<Fixture_object>  objects;
	std::vector<Fixture_operation> operations;
};

struct Comparison_event {
	int         chunk_x{};
	int         chunk_y{};
	std::string new_id;
	std::string existing_id;
	int         raw{};
	int         final{};
};

using Edge = std::pair<std::string, std::string>;

std::string field_value(const std::string& line, const std::string& name) {
	const std::string key = "\"" + name + "\"";
	size_t            pos = line.find(key);
	if (pos == std::string::npos) {
		throw std::runtime_error("Missing JSONL field '" + name + "': " + line);
	}
	pos = line.find(':', pos + key.size());
	if (pos == std::string::npos) {
		throw std::runtime_error("Malformed JSONL field '" + name + "': " + line);
	}
	++pos;
	while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
		++pos;
	}
	if (pos == line.size()) {
		throw std::runtime_error("Missing JSONL value for '" + name + "'.");
	}
	if (line[pos] == '"') {
		const size_t end = line.find('"', pos + 1);
		if (end == std::string::npos || line.find('\\', pos + 1) < end) {
			throw std::runtime_error("Fixture strings must be unescaped ASCII values: " + name);
		}
		return line.substr(pos + 1, end - pos - 1);
	}
	size_t end = line.find_first_of(",}", pos);
	if (end == std::string::npos) {
		end = line.size();
	}
	return line.substr(pos, end - pos);
}

int int_field(const std::string& line, const std::string& name) {
	const std::string value = field_value(line, name);
	size_t            consumed{};
	const int         parsed = std::stoi(value, &consumed);
	if (consumed != value.size()) {
		throw std::runtime_error("Invalid integer field '" + name + "'.");
	}
	return parsed;
}

bool bool_field(const std::string& line, const std::string& name) {
	const std::string value = field_value(line, name);
	if (value == "true") {
		return true;
	}
	if (value == "false") {
		return false;
	}
	throw std::runtime_error("Invalid boolean field '" + name + "'.");
}

std::vector<Fixture_case> read_fixture(const std::string& path) {
	std::ifstream input(path);
	if (!input) {
		throw std::runtime_error("Cannot open world-ordering fixture: " + path);
	}
	std::vector<Fixture_case> cases;
	Fixture_case*             current = nullptr;
	std::string               line;
	int                       line_number = 0;
	while (std::getline(input, line)) {
		++line_number;
		if (line.empty()) {
			continue;
		}
		try {
			if (int_field(line, "schemaVersion") != 1) {
				throw std::runtime_error("Unsupported schemaVersion.");
			}
			const std::string type = field_value(line, "type");
			if (type == "scenario") {
				if (current != nullptr) {
					throw std::runtime_error("Nested scenario record.");
				}
				cases.emplace_back();
				current = &cases.back();
				current->name         = field_value(line, "name");
				current->game         = field_value(line, "game");
				current->base_tile_x  = int_field(line, "baseTileX");
				current->base_tile_y  = int_field(line, "baseTileY");
			} else if (type == "object" && current != nullptr) {
				Fixture_object object;
				object.id       = field_value(line, "id");
				object.shape    = int_field(line, "shape");
				object.frame    = int_field(line, "frame");
				object.tile_x   = int_field(line, "tileX");
				object.tile_y   = int_field(line, "tileY");
				object.lift     = int_field(line, "lift");
				object.x_tiles  = int_field(line, "xTiles");
				object.y_tiles  = int_field(line, "yTiles");
				object.height   = int_field(line, "height");
				object.occludes = bool_field(line, "occludes");
				current->objects.push_back(std::move(object));
			} else if (type == "operation" && current != nullptr) {
				Fixture_operation operation;
				operation.name = field_value(line, "operation");
				if (operation.name != "paint") {
					operation.id = field_value(line, "id");
				}
				if (operation.name == "move") {
					operation.tile_x = int_field(line, "tileX");
					operation.tile_y = int_field(line, "tileY");
					operation.lift   = int_field(line, "lift");
				}
				if (operation.name == "compare") {
					operation.other_id = field_value(line, "otherId");
				}
				current->operations.push_back(std::move(operation));
			} else if (type == "endScenario" && current != nullptr) {
				current = nullptr;
			} else {
				throw std::runtime_error("Record type is not valid in this position: " + type);
			}
		} catch (const std::exception& error) {
			throw std::runtime_error(path + ":" + std::to_string(line_number) + ": " + error.what());
		}
	}
	if (current != nullptr || cases.empty()) {
		throw std::runtime_error("Fixture has an unterminated or empty scenario.");
	}
	return cases;
}

std::string json_string(const std::string& value) {
	std::ostringstream output;
	output << '"';
	for (const unsigned char c : value) {
		if (c == '"' || c == '\\') {
			output << '\\' << static_cast<char>(c);
		} else if (c >= 0x20) {
			output << static_cast<char>(c);
		}
	}
	output << '"';
	return output.str();
}

std::string sorted_string_array(std::vector<std::string> values) {
	std::sort(values.begin(), values.end());
	std::ostringstream output;
	output << '[';
	for (size_t i = 0; i < values.size(); ++i) {
		if (i) {
			output << ',';
		}
		output << json_string(values[i]);
	}
	output << ']';
	return output.str();
}

std::string ordered_string_array(const std::vector<std::string>& values) {
	std::ostringstream output;
	output << '[';
	for (size_t i = 0; i < values.size(); ++i) {
		if (i) {
			output << ',';
		}
		output << json_string(values[i]);
	}
	output << ']';
	return output.str();
}

std::string edge_array(const std::set<Edge>& edges) {
	std::ostringstream output;
	output << '[';
	bool first = true;
	for (const auto& [dependent, dependency] : edges) {
		if (!first) {
			output << ',';
		}
		first = false;
		output << "{\"dependent\":" << json_string(dependent) << ",\"dependency\":" << json_string(dependency) << '}';
	}
	output << ']';
	return output.str();
}

int chunk_for_tile(int tile) {
	return tile / c_tiles_per_chunk;
}

}    // namespace

struct World_ordering_lifecycle_harness::State {
	Game_window* window{};
	Game_map*    map{};
	std::map<std::string, Game_object_shared> objects;
	std::set<std::string> fixture_ids;
	std::unordered_map<Game_object*, std::string> ids;
	std::set<std::pair<int, int>> snapshot_chunks;
	std::vector<Comparison_event> comparison_calls;
	std::vector<std::pair<int, int>> paint_chunk_order;
	std::vector<std::string> paint_order;
	std::vector<std::string> direct_comparisons;
	std::set<Edge> previous_edges;
	int paint_chunk_x{};
	int paint_chunk_y{};
};

World_ordering_lifecycle_harness::State* World_ordering_lifecycle_harness::active_state = nullptr;

int World_ordering_lifecycle_harness::run(Game_window* window, const std::string& fixture_path, const std::string& output_path) {
	State state;
	try {
		if (window == nullptr) {
			throw std::runtime_error("Game_window is not initialized.");
		}
		const auto cases = read_fixture(fixture_path);
		std::ofstream output(output_path, std::ios::trunc);
		if (!output) {
			throw std::runtime_error("Cannot create world-ordering trace: " + output_path);
		}
		state.window = window;
		state.map    = window->get_map();
		active_state = &state;
		Map_chunk::ordering_comparison_callback = nullptr;
		Game_render* render = window->get_render();
		render->diagnostic_chunk_visit  = &World_ordering_lifecycle_harness::on_paint_chunk;
		render->diagnostic_object_visit = &World_ordering_lifecycle_harness::on_paint_object;

		output << "{\"schemaVersion\":1,\"type\":\"header\",\"exultCommit\":" << json_string(kExultSourceCommit)
			   << ",\"harnessCommit\":" << json_string(GIT_REVISION)
			   << ",\"game\":";
		output << json_string(Game::get_game_type() == SERPENT_ISLE ? "si" : "bg") << "}\n";

		int step = 0;
		for (const Fixture_case& fixture_case : cases) {
			if ((fixture_case.game == "si") != (Game::get_game_type() == SERPENT_ISLE)) {
				throw std::runtime_error("Fixture game does not match the selected Exult runtime: " + fixture_case.name);
			}
			if (fixture_case.game != "bg" && fixture_case.game != "si") {
				throw std::runtime_error("Unsupported fixture game: " + fixture_case.game);
			}
			state.objects.clear();
			state.ids.clear();
			state.fixture_ids.clear();
			state.comparison_calls.clear();
			state.paint_order.clear();
			state.paint_chunk_order.clear();
			state.direct_comparisons.clear();
			state.previous_edges.clear();
			state.snapshot_chunks.clear();
			Map_chunk::ordering_comparison_callback = nullptr;

			const int view_tiles_x = std::max(1, window->get_width() / c_tilesize);
			const int view_tiles_y = std::max(1, window->get_height() / c_tilesize);
			window->set_scrolls(fixture_case.base_tile_x - view_tiles_x / 2, fixture_case.base_tile_y - view_tiles_y / 2);

			// Assign deterministic handles to pre-existing game objects before test insertions change list positions.
			for (int cx = 0; cx < c_num_chunks; ++cx) {
				for (int cy = 0; cy < c_num_chunks; ++cy) {
					Map_chunk* chunk = state.map->get_chunk_unsafe(cx, cy);
					if (chunk == nullptr) {
						continue;
					}
					Object_iterator objects(chunk->get_objects());
					int             ordinal = 0;
					while (Game_object* object = objects.get_next()) {
						if (state.ids.find(object) == state.ids.end()) {
							std::ostringstream id;
							id << "native:" << cx << ':' << cy << ':' << ordinal;
							state.ids.emplace(object, id.str());
						}
						++ordinal;
					}
				}
			}

			for (const Fixture_object& definition : fixture_case.objects) {
				if (definition.id.empty() || state.objects.find(definition.id) != state.objects.end()) {
					throw std::runtime_error("Empty or duplicate fixture object ID in " + fixture_case.name);
				}
				if (definition.x_tiles < 1 || definition.y_tiles < 1 || definition.height < 0) {
					throw std::runtime_error("Invalid fixture shape dimensions for " + definition.id);
				}
				if (!Shape_manager::get_instance()->get_shapes().has_info(definition.shape)) {
					throw std::runtime_error("Fixture shape has no Shape_info record: " + std::to_string(definition.shape));
				}
				bool shape_is_used_by_world = false;
				for (const auto& [known_object, known_id] : state.ids) {
					ignore_unused_variable_warning(known_id);
					if (known_object->get_shapenum() == definition.shape) {
						shape_is_used_by_world = true;
						break;
					}
				}
				const Shape_info& info = ShapeID::get_info(definition.shape);
				const int actual_x_tiles = info.get_3d_xtiles(definition.frame);
				const int actual_y_tiles = info.get_3d_ytiles(definition.frame);
				const int actual_height  = info.get_3d_height();
				if (actual_x_tiles != definition.x_tiles || actual_y_tiles != definition.y_tiles ||
						actual_height != definition.height || info.occludes() != definition.occludes) {
					std::ostringstream detail;
					detail << "Fixture geometry mismatch for shape/frame " << definition.shape << '/' << definition.frame
						   << ": expected xtiles=" << definition.x_tiles << " ytiles=" << definition.y_tiles
						   << " height=" << definition.height << " occludes=" << definition.occludes
						   << ", Exult data has xtiles=" << actual_x_tiles << " ytiles=" << actual_y_tiles
						   << " height=" << actual_height << " occludes=" << info.occludes()
						   << (shape_is_used_by_world ? " (shape currently occurs in loaded world)" : "");
					throw std::runtime_error(detail.str());
				}
				auto object = std::make_shared<Ifix_game_object>(
						definition.shape,
						definition.frame,
						definition.tile_x % c_tiles_per_chunk,
						definition.tile_y % c_tiles_per_chunk,
						definition.lift);
				if (object->get_shape() == nullptr) {
					throw std::runtime_error("Fixture shape/frame is absent from the installed Exult game data: " + definition.id);
				}
				const std::string stable_id = fixture_case.name + ':' + definition.id;
				state.ids.emplace(object.get(), stable_id);
				state.fixture_ids.insert(stable_id);
				state.objects.emplace(definition.id, std::move(object));
			}
			Map_chunk::ordering_comparison_callback = &World_ordering_lifecycle_harness::on_comparison;

			for (const Fixture_operation& operation : fixture_case.operations) {
				state.comparison_calls.clear();
				state.direct_comparisons.clear();
				const auto mark_region = [&state](int cx, int cy) {
					for (int dx = -1; dx <= 1; ++dx) {
						for (int dy = -1; dy <= 1; ++dy) {
							if (cx + dx >= 0 && cx + dx < c_num_chunks && cy + dy >= 0 && cy + dy < c_num_chunks) {
								state.snapshot_chunks.emplace(cx + dx, cy + dy);
							}
						}
					}
				};
				const auto definition = std::find_if(
					fixture_case.objects.begin(), fixture_case.objects.end(),
					[&operation](const Fixture_object& candidate) { return candidate.id == operation.id; });
				if (definition != fixture_case.objects.end()) {
					mark_region(chunk_for_tile(definition->tile_x), chunk_for_tile(definition->tile_y));
				}
				const auto object_before = state.objects.find(operation.id);
				if (object_before != state.objects.end() && object_before->second->get_chunk() != nullptr) {
					mark_region(object_before->second->get_chunk()->get_cx(), object_before->second->get_chunk()->get_cy());
				}
				if (operation.name == "move") {
					mark_region(chunk_for_tile(operation.tile_x), chunk_for_tile(operation.tile_y));
				}
				if (operation.name == "compare") {
					const auto other = state.objects.find(operation.other_id);
					if (other == state.objects.end()) {
						throw std::runtime_error("Compare operation has no other object: " + operation.other_id);
					}
					if (other->second->get_chunk() != nullptr) {
						mark_region(other->second->get_chunk()->get_cx(), other->second->get_chunk()->get_cy());
					}
				}
				if (operation.name == "add") {
					auto& object = state.objects.at(operation.id);
					if (object->get_chunk() != nullptr) {
						throw std::runtime_error("Object is already in a chunk: " + operation.id);
					}
					if (definition == fixture_case.objects.end()) {
						throw std::runtime_error("Add operation has no object definition: " + operation.id);
					}
					Map_chunk* chunk = state.map->get_chunk_safely(
							chunk_for_tile(definition->tile_x), chunk_for_tile(definition->tile_y));
					if (chunk == nullptr) {
						throw std::runtime_error("Add target is outside the Exult world: " + operation.id);
					}
					chunk->add(object.get());
				} else if (operation.name == "remove") {
					auto& object = state.objects.at(operation.id);
					if (object->get_chunk() == nullptr) {
						throw std::runtime_error("Cannot remove an object outside a chunk: " + operation.id);
					}
					object->get_chunk()->remove(object.get());
				} else if (operation.name == "move") {
					auto& object = state.objects.at(operation.id);
					if (object->get_chunk() == nullptr) {
						throw std::runtime_error("Cannot move an object outside a chunk: " + operation.id);
					}
					object->Game_object::move(operation.tile_x, operation.tile_y, operation.lift, -1);
				} else if (operation.name == "compare") {
					auto& object_a = state.objects.at(operation.id);
					auto& object_b = state.objects.at(operation.other_id);
					Ordering_info info_a(window, object_a.get());
					Ordering_info info_b(window, object_b.get());
					const int ab = Game_object::compare(info_a, object_b.get());
					const int ba = Game_object::compare(info_b, object_a.get());
					std::ostringstream compare;
					compare << "{\"a\":" << json_string(fixture_case.name + ':' + operation.id)
							<< ",\"b\":" << json_string(fixture_case.name + ':' + operation.other_id)
							<< ",\"geometryA\":[" << info_a.xs << ',' << info_a.ys << ',' << info_a.zs
							<< "],\"geometryB\":[" << info_b.xs << ',' << info_b.ys << ',' << info_b.zs << ']'
							<< ",\"areaA\":[" << info_a.area.x << ',' << info_a.area.y << ',' << info_a.area.w << ',' << info_a.area.h
							<< "],\"areaB\":[" << info_b.area.x << ',' << info_b.area.y << ',' << info_b.area.w << ',' << info_b.area.h
							<< "],\"ab\":" << ab << ",\"ba\":" << ba << '}';
					state.direct_comparisons.push_back(compare.str());
				} else if (operation.name != "paint") {
					throw std::runtime_error("Unsupported operation '" + operation.name + "'.");
				}

				const auto object_after = state.objects.find(operation.id);
				if (object_after != state.objects.end() && object_after->second->get_chunk() != nullptr) {
					mark_region(object_after->second->get_chunk()->get_cx(), object_after->second->get_chunk()->get_cy());
				}

				state.paint_order.clear();
				state.paint_chunk_order.clear();
				render->paint_map(0, 0, window->get_width(), window->get_height());
				for (const auto& [cx, cy] : state.paint_chunk_order) {
					state.snapshot_chunks.emplace(cx, cy);
				}
				write_snapshot(output, fixture_case.name, step++, operation.name, operation.id);
				output.flush();
			}

			for (const auto& [id, object] : state.objects) {
				if (object->get_chunk() != nullptr) {
					throw std::runtime_error("Fixture did not remove object before scenario end: " + id);
				}
			}
		}

		render->diagnostic_chunk_visit  = nullptr;
		render->diagnostic_object_visit = nullptr;
		Map_chunk::ordering_comparison_callback = nullptr;
		active_state = nullptr;
		return 0;
	} catch (const std::exception& error) {
		if (active_state != nullptr && active_state->window != nullptr) {
			Game_render* render = active_state->window->get_render();
			render->diagnostic_chunk_visit  = nullptr;
			render->diagnostic_object_visit = nullptr;
		}
		Map_chunk::ordering_comparison_callback = nullptr;
		active_state = nullptr;
		std::cerr << "World-ordering lifecycle harness failed: " << error.what() << std::endl;
		return 2;
	}
}

void World_ordering_lifecycle_harness::on_comparison(
		Map_chunk* chunk, Game_object* newobj, Game_object* existing, int raw, int final) {
	if (active_state == nullptr) {
		return;
	}
	active_state->comparison_calls.push_back({
			chunk->cx,
			chunk->cy,
			active_state->ids.at(newobj),
			active_state->ids.at(existing),
			raw,
			final});
}

void World_ordering_lifecycle_harness::on_paint_chunk(int cx, int cy) {
	if (active_state != nullptr) {
		active_state->paint_chunk_order.emplace_back(cx, cy);
	}
}

void World_ordering_lifecycle_harness::on_paint_object(Game_object* object) {
	if (active_state != nullptr && active_state->fixture_ids.find(active_state->ids.at(object)) != active_state->fixture_ids.end()) {
		active_state->paint_order.push_back(active_state->ids.at(object));
	}
}

std::string World_ordering_lifecycle_harness::id_for(Game_object* object) {
	return active_state->ids.at(object);
}

void World_ordering_lifecycle_harness::write_snapshot(
		std::ostream& output, const std::string& fixture_name, int step, const std::string& operation, const std::string& id) {
	std::set<Edge> edges;
	std::vector<std::string> object_states;
	std::vector<std::string> chunks;
	for (const auto& [cx, cy] : active_state->snapshot_chunks) {
		Map_chunk* chunk = active_state->map->get_chunk_unsafe(cx, cy);
		if (chunk == nullptr) {
			std::ostringstream empty;
			empty << "{\"x\":" << cx << ",\"y\":" << cy << ",\"fromBelow\":0,\"fromRight\":0,\"fromBelowRight\":0,\"objects\":[]}";
			chunks.push_back(empty.str());
			continue;
		}
		std::vector<std::string> object_ids;
		Object_iterator        iterator(chunk->get_objects());
		while (Game_object* object = iterator.get_next()) {
			const std::string object_id = id_for(object);
			object_ids.push_back(object_id);
			std::vector<std::string> dependencies;
			std::vector<std::string> dependors;
			for (Game_object* dependency : object->dependencies) {
				const std::string id = id_for(dependency);
				dependencies.push_back(id);
				edges.emplace(object_id, id);
			}
			for (Game_object* dependor : object->dependors) {
				dependors.push_back(id_for(dependor));
			}
			if (active_state->fixture_ids.find(object_id) != active_state->fixture_ids.end()) {
				Ordering_info ordering(active_state->window, object);
				const Shape_info& info = object->get_info();
				const int frame = object->get_framenum();
				const Tile_coord tile = object->get_tile();
				std::ostringstream state;
				state << "{\"id\":" << json_string(object_id)
					  << ",\"shape\":" << object->get_shapenum()
					  << ",\"frame\":" << frame
					  << ",\"tileX\":" << tile.tx
					  << ",\"tileY\":" << tile.ty
					  << ",\"lift\":" << tile.tz
					  << ",\"xTiles\":" << info.get_3d_xtiles(frame)
					  << ",\"yTiles\":" << info.get_3d_ytiles(frame)
					  << ",\"height\":" << info.get_3d_height()
					  << ",\"occludes\":" << (info.occludes() ? "true" : "false")
					  << ",\"area\":[" << ordering.area.x << ',' << ordering.area.y << ',' << ordering.area.w << ',' << ordering.area.h << ']'
					  << ",\"dependencies\":" << sorted_string_array(std::move(dependencies))
					  << ",\"dependors\":" << sorted_string_array(std::move(dependors)) << '}';
				object_states.push_back(state.str());
			}
		}
		std::ostringstream chunk_state;
		chunk_state << "{\"x\":" << cx << ",\"y\":" << cy
					<< ",\"fromBelow\":" << static_cast<int>(chunk->from_below)
					<< ",\"fromRight\":" << static_cast<int>(chunk->from_right)
					<< ",\"fromBelowRight\":" << static_cast<int>(chunk->from_below_right)
						<< ",\"objects\":" << ordered_string_array(object_ids) << '}';
		chunks.push_back(chunk_state.str());
	}
	std::set<Edge> added;
	std::set<Edge> removed;
	std::set_difference(edges.begin(), edges.end(), active_state->previous_edges.begin(), active_state->previous_edges.end(),
			std::inserter(added, added.end()));
	std::set_difference(active_state->previous_edges.begin(), active_state->previous_edges.end(), edges.begin(), edges.end(),
			std::inserter(removed, removed.end()));
	active_state->previous_edges = edges;

	std::vector<std::string> paint_chunks;
	for (const auto& [cx, cy] : active_state->paint_chunk_order) {
		paint_chunks.push_back("[" + std::to_string(cx) + ',' + std::to_string(cy) + "]");
	}
	std::ostringstream output_line;
	output_line << "{\"schemaVersion\":1,\"type\":\"step\",\"scenario\":" << json_string(fixture_name)
				<< ",\"step\":" << step
				<< ",\"operation\":" << json_string(operation)
				<< ",\"id\":" << json_string(id)
				<< ",\"chunks\":[";
	for (size_t i = 0; i < chunks.size(); ++i) {
		if (i) {
			output_line << ',';
		}
		output_line << chunks[i];
	}
	output_line << "],\"objectStates\":[";
	for (size_t i = 0; i < object_states.size(); ++i) {
		if (i) {
			output_line << ',';
		}
		output_line << object_states[i];
	}
	output_line << "],\"edges\":" << edge_array(edges)
			   << ",\"addedEdges\":" << edge_array(added)
			   << ",\"removedEdges\":" << edge_array(removed)
			   << ",\"comparisonCalls\":[";
	for (size_t i = 0; i < active_state->comparison_calls.size(); ++i) {
		if (i) {
			output_line << ',';
		}
		const Comparison_event& event = active_state->comparison_calls[i];
		output_line << "{\"chunkX\":" << event.chunk_x << ",\"chunkY\":" << event.chunk_y
					<< ",\"newObject\":" << json_string(event.new_id)
					<< ",\"existingObject\":" << json_string(event.existing_id)
					<< ",\"compare\":" << event.raw << ",\"finalCompare\":" << event.final << '}';
	}
	output_line << "],\"paintChunkOrder\":[";
	for (size_t i = 0; i < paint_chunks.size(); ++i) {
		if (i) {
			output_line << ',';
		}
		output_line << paint_chunks[i];
	}
	output_line << "],\"fixturePaintOrder\":" << ordered_string_array(active_state->paint_order)
			   << ",\"directComparisons\":[";
	for (size_t i = 0; i < active_state->direct_comparisons.size(); ++i) {
		if (i) {
			output_line << ',';
		}
		output_line << active_state->direct_comparisons[i];
	}
	output_line << "]}";
	output << output_line.str() << '\n';
}
