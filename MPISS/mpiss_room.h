#pragma once
#ifndef MPISS_ROOM_NDEF
#define MPISS_ROOM_NDEF

#include "mpiss_cell.h"
#include "mpiss_cemetery.h"

namespace mpiss {

	struct room {
		double* contact_probability;
		point<state_enum_size>* state_spread_modifier;
		cemetery* closest_cemetery;
		std::vector<cell*> cells;
		std::vector<size_t> _buffer_list;
		size_t counters[state_enum_size];
		room(
			double* contact_probability,
			point<state_enum_size>* state_spread_modifier,
			cemetery* closest_cemetery
		) : contact_probability(contact_probability),
			state_spread_modifier(state_spread_modifier),
			closest_cemetery(closest_cemetery)
		{
			clear_counters();
		}
		void make_iteration() {
			clear_counters();
			for (auto cell : cells) 
				counters[(size_t)cell->cur_disease_state]++;
			for (auto it = cells.begin(); it != cells.end(); it++) {
				double rnd = erand();
				if (rnd < *contact_probability) {
					size_t rnd_id = erand() * cells.size();
					single_contact(it, cells.begin() + rnd_id);
				}
			}
			for (auto cell : cells) 
				cell->make_iteration();
			for (auto it = cells.begin(); it != cells.end(); it++) {
				if ((*it)->cur_disease_state == disease_state::dead)
					_buffer_list.push_back(it - cells.begin());
			}
			for (auto entry_rit = _buffer_list.rbegin(); entry_rit != _buffer_list.rend(); entry_rit++)
				closest_cemetery->take_from(cells, *entry_rit);
			for (auto& cur_cell : cells)
				cur_cell->set_next_iter_state();
			_buffer_list.clear();
		}
		void clear_counters() {
			for (int i = 0; i < state_enum_size; i++) 
				counters[i] = 0;
		}
		void print_counters(std::ostream& out) const {
			for (int i = 0; i < state_enum_size; i++)
				out << magic_enum::enum_name<disease_state>((disease_state)i) << ": " << counters[i] << std::endl;
		}
		size_t get_sick_count() const {
			return
				counters[(size_t)mpiss::disease_state::hidden_nonspreading] +
				counters[(size_t)mpiss::disease_state::hidden_spreading] +
				counters[(size_t)mpiss::disease_state::active_spread];
		}
		void single_contact(std::vector<cell*>::iterator a_cell_id, std::vector<cell*>::iterator b_cell_id) {
			bool a_or_b = (bool)(*a_cell_id)->cur_disease_state || (bool)(*b_cell_id)->cur_disease_state;
			bool a_and_b = (bool)(*a_cell_id)->cur_disease_state && (bool)(*b_cell_id)->cur_disease_state;
			if (a_or_b && !a_and_b) {
				if ((bool)(*b_cell_id)->cur_disease_state)
					std::swap(a_cell_id, b_cell_id);
				auto t = erand();
				if (t < (state_spread_modifier->operator[]((size_t)(*a_cell_id)->cur_disease_state))) {
					(*b_cell_id)->next_disease_state = disease_state::hidden_nonspreading;
				}
			}
		}
	};
}

#endif