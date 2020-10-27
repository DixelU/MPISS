#pragma once
#ifndef MPISS_TOWN_NDEF
#define MPISS_TOWN_NDEF

#include <unordered_map>

#include "mpiss_cell.h"
#include "mpiss_room.h"
#include "mpiss_sheduled_cell.h"

#define CAST_SHEDULED(cell) (*(sheduled_cell*)(&cell))

namespace mpiss {
	struct town {
		cemetery *cem;
		size_t avg_counter[state_enum_size];
		std::unordered_map<shedule_place, std::vector<room>> places;

		town(cemetery *cem, const std::unordered_map<shedule_place, std::vector<room>>& places): places(places) {

		}
		void reset() {
			for (auto& dead : cem->deads) {
				dead->reset();
				auto dead_sh = &CAST_SHEDULED(dead);
				auto it = std::find_if(
					dead_sh->shedule->begin(), 
					dead_sh->shedule->end(),
					[](const shedule_ticket& t) -> bool {
						return t.type == shedule_place::home; 
					}
				);
				places[shedule_place::home][it->id].cells.push_back(dead_sh);
			}
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					for (auto& c_cell : single_place.cells) {
						c_cell->reset();
					}
				}
			}
		}
		void make_iteration() {
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					for (auto& c_cell : single_place.cells) {
						auto& s_cell = CAST_SHEDULED(c_cell);
						if (s_cell.cur_shedule != s_cell.prev_shedule) {
							auto& shedule = s_cell.shedule->operator[](s_cell.cur_shedule);
							/// TODO: move to according shedule place.
						}
					}
				}
			}
			/// TODO: finish iteration.
			/// and finish "average counters"
		}
	};
}

#endif