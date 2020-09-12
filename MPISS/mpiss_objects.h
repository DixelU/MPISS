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
		kid, teen, mature, old
		, null
	};
	enum class disease_state {
		healthy, hidden_nonspreading, hidden_spread, active_spread, immune, dead
		, null
	};

	template<typename T>
	inline constexpr size_t size_of_enum() {
		return (size_t)(T::null);
	}

	constexpr auto type_enum_size = size_of_enum<mpiss::age_type>();
	constexpr auto state_enum_size = size_of_enum<mpiss::disease_state>();
	constexpr auto max_enums_size = std::max(type_enum_size, state_enum_size);
	constexpr float neg_inf = -INFINITY;
	constexpr float pos_inf = INFINITY;

	sq_matrix<type_enum_size> contact_matrix;

	struct __prob_line {
		const float s, o;
		//__prob_line(float s, float o):s(s),o(o) {}
		float eval(const float& x) const {
			return (-1 / s) * (x - o);
		}
	};

	struct disease_progress_line {
		std::vector<std::pair<mpiss::disease_state, __prob_line>> data;
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
		mpiss::disease_state update_disease_state(mpiss::age_type age_t, const size_t& time_since_contact, const float& val) const {
			return data[(size_t)age_t].update_disease_state(time_since_contact, val);
		}
	};

	struct cell {
		mpiss::age_type age_t;
		disease_state cell_state;
		int64_t time_since_contact;
		aged_disease_progress_line* adpl;
		float value;
		cell(aged_disease_progress_line* adpl,mpiss::age_type age_t = mpiss::age_type::mature):
			age_t(age_t), cell_state(mpiss::disease_state::healthy), time_since_contact(-1) {
			value = erand();
		}
		void make_iteration() {
			if (cell_state == mpiss::disease_state::dead)
				return;
			bool is = (cell_state != disease_state::healthy);
			time_since_contact += is;
			auto new_state = adpl->update_disease_state(age_t, time_since_contact, value);
			if (new_state == mpiss::disease_state::healthy && new_state != cell_state) 
				time_since_contact = -1;
			cell_state = new_state;
		}
	};

	struct room {
		sq_matrix<type_enum_size>* contact_matrix_ptr;
		std::vector<cell*> cells;

	};

}
#endif