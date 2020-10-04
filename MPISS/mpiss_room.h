#pragma once
#ifndef MPISS_ROOM_NDEF
#define MPISS_ROOM_NDEF

#include "mpiss_cell.h"
#include "mpiss_cemetery.h"

namespace mpiss {

	struct room {
		double* contact_probability;
		sq_matrix<age_enum_size>* spread_matrix_ptr;
		point<state_enum_size>* state_spread_modifier;
		cemetery* closest_cemetery;
		std::vector<cell*> cells;
		std::vector<size_t> _buffer_list;
		size_t counters[state_enum_size];
		room(
			double* contact_probability,
			sq_matrix<age_enum_size>* spread_matrix_ptr,
			point<state_enum_size>* state_spread_modifier,
			cemetery* closest_cemetery
		) : contact_probability(contact_probability),
			spread_matrix_ptr(spread_matrix_ptr),
			state_spread_modifier(state_spread_modifier),
			closest_cemetery(closest_cemetery)
		{
			clear_counters();
		}
		void make_iteration() {
			auto it = cells.begin();
			clear_counters();
			for (; it != cells.end(); it++) {
				(*it)->make_iteration();

				double rnd = erand();
				if (rnd < *contact_probability) {
					size_t rnd_id = erand() * cells.size();
					single_contact(cells.begin() + rnd_id, it);
				}

				if ((*it)->cur_disease_state == disease_state::dead)
					_buffer_list.push_back(it - cells.begin());

				counters[(size_t)(*it)->cur_disease_state]++;
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
			if ((bool)(*a_cell_id)->cur_disease_state || (bool)(*b_cell_id)->cur_disease_state) {
				if (!(bool)(*a_cell_id)->cur_disease_state)
					std::swap(a_cell_id, b_cell_id);
				double t = erand() / (state_spread_modifier->operator[]((size_t)(*a_cell_id)->cur_disease_state));
				if (t < spread_matrix_ptr->at((size_t)(*a_cell_id)->age_t, (size_t)(*b_cell_id)->age_t) && !(bool)(*b_cell_id)->cur_disease_state) {
					(*b_cell_id)->next_disease_state = disease_state::hidden_nonspreading;
				}
			}
		}
	};
}

#endif