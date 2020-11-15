#pragma once
#ifndef MPISS_TOWN_NDEF
#define MPISS_TOWN_NDEF

#include <unordered_map>

#include "mpiss_room.h"
#include "mpiss_sheduled_cell.h"

#define CAST_SHEDULED(cell) (*(sheduled_cell*)(&cell))
#define CAST_PSHEDULED(cell) ((sheduled_cell*)(&cell))

namespace mpiss {
	struct town {
		cemetery *cem;
		size_t avg_counter[state_enum_size];
		std::unordered_map<shedule_place, std::vector<room>> places;
		size_t counters[state_enum_size];

		town(cemetery *cem, const std::unordered_map<shedule_place, std::vector<room>>& places): places(places), cem(cem) {
			update_counters();
		}
		void reset() {
			for (auto& dead : cem->deads) {
				dead->reset();
				auto dead_sh = CAST_PSHEDULED(*dead);
				auto it = std::find_if(
					(*(dead_sh->shedule)).begin(),
					(*(dead_sh->shedule)).end(),
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
			move_cells();
		}
		void update_counters() {
			for (int dis_st = 0; dis_st < state_enum_size; dis_st++) 
				counters[dis_st] = 0;
			for (auto& place_type : places) 
				for (auto& single_place : place_type.second) 
					for (int dis_st = 0; dis_st < state_enum_size; dis_st++) 
						counters[dis_st] += single_place.counters[dis_st];
		}
		void move_cells() {
			bool flag_back_move = false;
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					flag_back_move = false;
					for (auto c_cell_it = single_place.cells.begin(); c_cell_it != single_place.cells.end() || flag_back_move; c_cell_it++) {
						if (flag_back_move) {
							flag_back_move = false;
							c_cell_it--;
						}
						auto s_cell = CAST_PSHEDULED(*c_cell_it);
						if (s_cell->cur_shedule != s_cell->prev_shedule) {
							flag_back_move = true;
							auto& shedule = (*s_cell->shedule)[s_cell->cur_shedule];
							auto cell_ptr = (sheduled_cell*)single_place.remove_cell_without_destroying(c_cell_it - single_place.cells.begin());
							auto& place_type_ref = places[shedule.type];
							double place_type_amount = place_type_ref.size();
							auto place_room_id = shedule.get_rand_id();
							place_room_id = std::clamp(place_room_id, 0., place_type_amount);
							place_type_ref[(size_t)place_room_id].cells.push_back(cell_ptr);
						}
					}
				}
			}
		}
		void make_iteration() {
			move_cells();
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					single_place.make_iteration();
				}
			}
		}
	};
}

#undef CAST_SHEDULED
#undef CAST_PSHEDULED

#endif