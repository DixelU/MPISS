#pragma once
#ifndef MPISS_OBJS
#define MPISS_OBJS

#include "multidimentional_point.h"
#include "magic_enum.hpp"
#include "matrix.h"

#include <list>
#include <vector>
#include <random>
#include <cmath>
#include <mutex>

namespace mpiss {
	namespace __utils {
		struct grand {
			std::random_device rd{};
			std::mt19937 gen{ rd() };
			std::normal_distribution<> d{ 0,1 };
			std::recursive_mutex locker;
		};
		grand __gr;


	}
	double nrand() {
		__utils::__gr.locker.lock();
		auto rval = __utils::__gr.d(__utils::__gr.gen);
		__utils::__gr.locker.unlock();
		return rval;
	}
	double erand() {
		return __utils::__gr.gen();
	}

	enum class age_type {
		kid = 0, teen = 1, mature = 2, old = 3
		, null = 4
	};
	enum class disease_state {
		healthy = 0, hidden_nonspreading = 1, hidden_spreading = 2, active_spread = 3, immune = 4, dead = 5
		, null = 6
	};

	template<typename T>
	inline constexpr size_t size_of_enum() {
		return (size_t)(T::null);
	}

	constexpr auto age_enum_size = size_of_enum<mpiss::age_type>();
	constexpr auto state_enum_size = size_of_enum<mpiss::disease_state>();
	constexpr auto max_enums_size = std::max(age_enum_size, state_enum_size);
	constexpr float neg_inf = -INFINITY;
	constexpr float pos_inf = INFINITY;

	struct __prob_line {
		const float s, o;
		__prob_line(float s, float o):s(s),o(o) {}
		float eval(const float& x) const {
			return (-1 / s) * (x - o);
		}
	};

	struct disease_progress_line {
		std::vector<std::pair<mpiss::disease_state, __prob_line>> data;
		disease_progress_line(const std::vector<std::pair<mpiss::disease_state, __prob_line>>& d = std::vector<std::pair<mpiss::disease_state, __prob_line>>()): data(d){}
		mpiss::disease_state update_disease_state(const size_t& time_since_contact, const float& val) const {
			if (val == neg_inf)
				return mpiss::disease_state::dead;
			mpiss::disease_state prev_state = mpiss::disease_state::healthy;
			for (const auto& pr : data) {
				float i_val = pr.second.eval((float)time_since_contact);
				if (i_val > val)
					return prev_state;
				prev_state = pr.first;
			}
		}
	};

	struct aged_disease_progress_line {
		std::vector<disease_progress_line> data;
		aged_disease_progress_line(const std::vector<disease_progress_line>& d = std::vector<disease_progress_line>()):data(d){}
		mpiss::disease_state update_disease_state(mpiss::age_type age_t, const size_t& time_since_contact, const float& val) const {
			return data[(size_t)age_t].update_disease_state(time_since_contact, val);
		}
	};

	struct cell {
		mpiss::age_type age_t;
		disease_state cur_disease_state;
		int64_t time_since_contact;
		aged_disease_progress_line* adpl;
		float value;
		cell(aged_disease_progress_line* adpl,mpiss::age_type age_t = mpiss::age_type::mature):
			age_t(age_t), cur_disease_state(mpiss::disease_state::healthy), time_since_contact(-1), adpl(adpl) {
			value = erand();
		}
		void make_iteration() {
			if (cur_disease_state == mpiss::disease_state::dead)
				return;
			bool is = (cur_disease_state != disease_state::healthy);
			time_since_contact += is;
			auto new_state = adpl->update_disease_state(age_t, time_since_contact, value);
			if (new_state != cur_disease_state) {
				value = erand();
				if (new_state == mpiss::disease_state::healthy)
					time_since_contact = -1;
				cur_disease_state = new_state;
			}
		}
	};

	struct cemetery {
		std::vector<cell*> deads;
		cemetery(){}
		void take_from(std::vector<cell*>& cells, size_t id) {
			size_t last_id = cells.size() - 1;
			std::swap(cells[id], cells[last_id]);
			deads.push_back(cells.back());
			cells.pop_back();
		}
	};

	struct room {
		double* contact_probability;
		sq_matrix<age_enum_size>* spread_matrix_ptr;
		point<state_enum_size>* state_spread_modifier;
		cemetery* closest_cemetery;
		std::vector<cell*> cells;
		std::vector<size_t> _buffer_list;
		size_t cur_ill_count;
		room(
			double* contact_probability, 
			sq_matrix<age_enum_size>* spread_matrix_ptr, 
			point<state_enum_size>* state_spread_modifier,
			cemetery* closest_cemetery
		) : contact_probability(contact_probability ), 
			spread_matrix_ptr(spread_matrix_ptr),
			state_spread_modifier(state_spread_modifier), 
			closest_cemetery(closest_cemetery),
			cur_ill_count(0)
		{  }
		void make_iteration() {
			auto it = cells.begin(); 
			cur_ill_count = 0;
			for (; it != cells.end(); it++) {
				(*it)->make_iteration();
				double rnd = erand();
				if (rnd < *contact_probability) {
					size_t rnd_id = erand() * cells.size();
					single_contact(cells.begin() + rnd_id, it);
				}

				if ((*it)->cur_disease_state == disease_state::dead) 
					_buffer_list.push_back(it - cells.begin());

				if ((*it)->cur_disease_state > disease_state::healthy && (*it)->cur_disease_state < disease_state::immune)
					cur_ill_count++;
			}
			for (auto entry : _buffer_list) 
				closest_cemetery->take_from(cells, entry);
			_buffer_list.clear();
		}
		void single_contact(std::vector<cell*>::iterator a_cell_id, std::vector<cell*>::iterator b_cell_id) {
			if ((bool)(*a_cell_id)->cur_disease_state || (bool)(*b_cell_id)->cur_disease_state) {
				if (!(bool)(*a_cell_id)->cur_disease_state)
					std::swap(a_cell_id, b_cell_id);
				double t = erand() / (state_spread_modifier->operator[]((size_t)(*a_cell_id)->cur_disease_state));
				if (t < spread_matrix_ptr->at((size_t)(*a_cell_id)->age_t, (size_t)(*b_cell_id)->age_t) && !(bool)(*b_cell_id)->cur_disease_state) {
					(*b_cell_id)->cur_disease_state = disease_state::hidden_nonspreading;
				}
			}
		}
	};

}
#endif