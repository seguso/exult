/**
 ** Iregobjs.cc - Ireg (moveable) game objects.
 **
 ** Written: 10/1/98 - JSF
 **/

/*
Copyright (C) 1998-2000  Jeffrey S. Freedman
Copyright (C) 2000-2022  The Exult Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "Gump_manager.h"
#include "cheat.h"
#include "chunks.h"
#include "contain.h"
#include "databuf.h"
#include "effects.h"
#include "gamemap.h"
#include "gamewin.h"
#include "ucsched.h"

#include <fstream>

using std::ostream;

/*
 *  Get chunk coords, or 255.
 */
inline int Game_object::get_cxi() const {
	return chunk ? chunk->cx : 255;
}

inline int Game_object::get_cyi() const {
	return chunk ? chunk->cy : 255;
}

/*
 *  Paint at given spot in world.
 */

void Ireg_game_object::paint() {
	int x;
	int y;
	gwin->get_shape_location(this, x, y);

	if (get_shapenum() == 440) {
		static int last_frame = -1;
		static int last_tx = -1;
		static int last_ty = -1;
		static int last_tz = -1;
		const Tile_coord t = get_tile();
		const int frame = get_framenum();
		if (frame != last_frame || t.tx != last_tx || t.ty != last_ty || t.tz != last_tz) {
			last_frame = frame;
			last_tx = t.tx;
			last_ty = t.ty;
			last_tz = t.tz;
			const Shape_info& info = get_info();
			std::ofstream out("exult-illumination-440.log", std::ios::out | std::ios::app);
			if (out.good()) {
				out << "shape=440"
					<< " frame=" << frame
					<< " tile=(" << t.tx << "," << t.ty << "," << t.tz << ")"
					<< " screen=(" << x << "," << y << ")"
					<< " animated=" << (info.is_animated() ? 1 : 0)
					<< " translucent=" << (info.has_translucency() ? 1 : 0)
					<< " light=" << info.get_object_light(frame)
					<< " frames=" << get_num_frames()
					<< " owner=" << (get_owner() ? 1 : 0)
					<< " shapeClass=" << static_cast<int>(info.get_shape_class())
					<< " hatchable=" << (info.get_shape_class() == Shape_info::hatchable ? 1 : 0);
				if (info.is_animated()) {
					const ShapeID sid(440, frame);
					const Animation_info* ai =
							info.get_animation_info_safe(440, sid.get_num_frames());
					if (ai) {
						out << " animType=" << static_cast<int>(ai->get_type())
							<< " frameDelay=" << ai->get_frame_delay()
							<< " freezeFirstChance=" << ai->get_freeze_first_chance()
							<< " recycle=" << ai->get_recycle();
					}
				}

				Game_object_vector lamps;
				Game_object::find_nearby(lamps, t, 526, 12, 0);
				out << " nearby526=" << lamps.size();
				for (auto* lamp : lamps) {
					if (!lamp) {
						continue;
					}
					const Tile_coord lt = lamp->get_tile();
					out << " [" << lamp->get_framenum()
						<< "@(" << lt.tx << "," << lt.ty << "," << lt.tz << ")]";
				}

				lamps.clear();
				Game_object::find_nearby(lamps, t, 889, 12, 0);
				out << " nearby889=" << lamps.size();
				for (auto* lamp : lamps) {
					if (!lamp) {
						continue;
					}
					const Tile_coord lt = lamp->get_tile();
					out << " [" << lamp->get_framenum()
						<< "@(" << lt.tx << "," << lt.ty << "," << lt.tz << ")]";
				}
				out << "\n";
			}
		}
	}

	if (flags & (1L << Obj_flags::invisible)) {
		paint_invisible(x, y);
	} else {
		paint_shape(x, y);
	}
}

/*
 *  Move to a new absolute location.  This should work even if the old
 *  location is invalid (cx=cy=255).
 */

void Ireg_game_object::move(int newtx, int newty, int newlift, int newmap) {
	if (owner) {    // Watch for this.
		owner->remove(this);
		set_invalid();    // So we can safely move it back.
	}
	Game_object::move(newtx, newty, newlift, newmap);
}

/*
 *  Remove an object from its container, or from the world.
 *  The object is deleted.
 */

void Ireg_game_object::remove_this(Game_object_shared* keep    // Non-null to not delete.
) {
	// Do this before all else.
	if (!keep) {
		cheat.clear_this_grabbed_actor(this->as_actor());    // Could be an actor
	} else {
		*keep = shared_from_this();
	}
	if (owner) {    // In a bag, box, or person.
		owner->remove(this);
	} else {    // In the outside world.
		if (chunk) {
			chunk->remove(this);
		}
	}
}

/*
 *  Can this be dragged?
 */

bool Ireg_game_object::is_dragable() const {
	// 0 weight means 'too heavy'.
	return get_info().get_weight() > 0;
}

/*
 *  Write the common IREG data for an entry.
 *  Note:  Length is incremented if this is an extended entry (shape# >
 *      1023).
 *  Output: ->past data written.
 */

unsigned char* Ireg_game_object::write_common_ireg(
		int            norm_len,    // Normal length (if not extended).
		unsigned char* buf          // Buffer to be filled.
) {
	unsigned char* endptr;
	const int      shapenum = get_shapenum();
	const int      framenum = get_framenum();
	if (shapenum >= 1024 || framenum >= 64) {
		Write1(buf, IREG_EXTENDED);
		norm_len++;
		buf[3] = shapenum & 0xff;
		buf[4] = shapenum >> 8;
		buf[5] = framenum;
		endptr = buf + 6;
	} else {
		if (get_lift() > 15) {
			Write1(buf, IREG_EXTENDED2);
		}
		buf[3] = shapenum & 0xff;
		buf[4] = ((shapenum >> 8) & 3) | (framenum << 2);
		endptr = buf + 5;
	}
	buf[0] = norm_len;
	if (owner) {
		// Coords within gump.
		buf[1] = get_tx();
		buf[2] = get_ty();
	} else {    // Coords on map.
		buf[1] = ((get_cxi() % 16) << 4) | get_tx();
		buf[2] = ((get_cyi() % 16) << 4) | get_ty();
	}
	return endptr;
}

/*
 *  Write out.
 */

void Ireg_game_object::write_ireg(ODataSource* out) {
	unsigned char buf[20];    // 10-byte entry;
	uint8*        ptr = write_common_ireg(10, buf);
	Write1(ptr, nibble_swap(get_lift()));
	const Shape_info& info  = get_info();
	uint8             value = get_quality();
	if (info.has_quality_flags()) {
		// Store 'quality_flags'.
		value = (get_flag(Obj_flags::invisible) ? 1 : 0) + (get_flag(Obj_flags::okay_to_take) ? (1 << 3) : 0);
	}
	// Special case for 'quantity' items:
	else if (get_flag(Obj_flags::okay_to_take) && info.has_quantity()) {
		value |= 0x80;
	}
	Write1(ptr, value);
	Write1(ptr, static_cast<uint8>(get_flag(Obj_flags::is_temporary)));
	Write1(ptr, 0);    // Filler, I guess.
	Write1(ptr, 0);
	Write1(ptr, 0);
	out->write(reinterpret_cast<char*>(buf), ptr - buf);
	// Write scheduled usecode.
	Game_map::write_scheduled(out, this);
}

// Get size of IREG. Returns -1 if can't write to buffer
int Ireg_game_object::get_ireg_size() {
	// These shouldn't ever happen, but you never know
	if (gumpman->find_gump(this) || Usecode_script::find(this)) {
		return -1;
	}
	return 6 + get_common_ireg_size();
}
