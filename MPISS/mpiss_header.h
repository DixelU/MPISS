#pragma once
#ifndef MPISS_HEADER_NDEF
#define MPISS_HEADER_NDEF

#include "multidimentional_point.h"
#include "magic_enum.hpp"
#include "matrix.h"

#include <list>
#include <iostream>
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
		return __utils::__gr.gen() / (double(0xFFFFFFFFu));
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


	struct disease_progress {
		virtual mpiss::disease_state update_disease_state(
			mpiss::age_type age_t,
			mpiss::disease_state prev_state,
			const int64_t& time_since_contact,
			const float& val
		) const {
			return prev_state;
		};
	};
}

#endif