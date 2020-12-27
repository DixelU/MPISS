#pragma once
#ifndef MPISS_TOWN_NDEF
#define MPISS_TOWN_NDEF

#include <unordered_map>
#include <functional>

#include "mpiss_room.h"
#include "mpiss_sheduled_cell.h"

#define CAST_SHEDULED(cell) (*(sheduled_cell*)(&cell))
#define CAST_PSHEDULED(cell) ((sheduled_cell*)(&cell))

namespace mpiss {
	struct town {
		cemetery *cem;
		std::unordered_map<shedule_place, std::vector<room>> places;
		point<state_enum_size> counters;

		std::vector<sheduled_cell*> lockdown_cells_colist;
		std::array<std::function<double(double)>, state_enum_size> dis_spread_modifiers;
		std::array<std::function<double(double)>, shedule_enum_size> place_spread_modifiers;
		std::array<std::function<double(double)>, state_enum_size> lockdown_drivers;
		void assign_default_functions() {
			for (auto& f : dis_spread_modifiers)
				f = [](double x) -> double { return 1; };
			for (auto& f : place_spread_modifiers)
				f = [](double x) -> double { return 1; };
			for (auto& f : lockdown_drivers)
				f = [](double x) -> double { return 0; };
			/// TODO: implement lockdown/params shift logic
		}
		town(cemetery *cem, const std::unordered_map<shedule_place, std::vector<room>>& places): places(places), cem(cem) {
			update_counters();
		}
		void reset() {
			for (auto& dead : cem->deads) {
				dead->reset();
				auto dead_sh = CAST_PSHEDULED(*dead);
				(dead_sh->shedule).revert_all_overrides();
				auto it = std::find_if(
					(*(dead_sh->shedule)).begin(),
					(*(dead_sh->shedule)).end(),
					[](const shedule_ticket& t) -> bool {
						return t.type == shedule_place::home; 
					}
				);
				places[shedule_place::home][it->id].cells.push_back(dead_sh);
			}
			cem->deads.clear();
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					for (auto& c_cell : single_place.cells) {
						c_cell->reset();
					}
				}
			}
			move_cells();
			update_counters();
		}
		void update_counters() {
			for (int dis_st = 0; dis_st < state_enum_size; dis_st++) 
				counters[dis_st] = 0;
			for (auto& place_type : places) 
				for (auto& single_place : place_type.second) 
					for (int dis_st = 0; dis_st < state_enum_size; dis_st++) 
						counters[dis_st] += single_place.counters[dis_st];
			counters[(size_t)mpiss::disease_state::dead] = cem->deads.size();
		}
		void move_cells() {
			bool flag_back_move = false;
			for (auto& place_type : places) {
				for (auto& single_place : place_type.second) {
					flag_back_move = false;
					for (int i = 0; i < single_place.cells.size();) {
						auto s_cell = (mpiss::sheduled_cell*)single_place.cells[i];
						if (s_cell->cur_shedule != s_cell->prev_shedule && (
								(*s_cell->shedule)[s_cell->cur_shedule].type != (*s_cell->shedule)[s_cell->prev_shedule].type ||
								(*s_cell->shedule)[s_cell->cur_shedule].id != (*s_cell->shedule)[s_cell->prev_shedule].id )) {

							auto cell_ptr = (sheduled_cell*)single_place.remove_cell_without_destroying(i);

							auto& shedule = (*cell_ptr->shedule)[cell_ptr->cur_shedule];
							auto& place_type_ref = places[shedule.type];
							double place_type_amount = place_type_ref.size();
							auto place_room_id = shedule.get_rand_id();
							place_room_id = std::clamp(place_room_id, 0., place_type_amount-1);
							cell_ptr->prev_shedule = cell_ptr->cur_shedule;
							place_type_ref[(size_t)place_room_id].cells.push_back(cell_ptr);
						}
						else
							i++;
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