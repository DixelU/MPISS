#pragma once
#ifndef MPISS_HEADER_NDEF
#define MPISS_HEADER_NDEF

#include "multidimentional_point.h"
#include "magic_enum.hpp"
//#include "matrix.h"

#include <list>
#include <iostream>
#include <vector>

#include <random>
#include <time.h>
#include <thread>

#include <cmath>
#include <mutex>

#if defined (_MSC_VER)  // Visual studio
#define thread_local __declspec( thread )
#elif defined (__GCC__) // GCC
#define thread_local __thread
#endif

namespace mpiss {
	namespace __utils {

		bool is_number(const std::wstring& s) { 
			auto it = s.begin();
			while (it != s.end() && std::isdigit(*it)) ++it;
			return s.size() && it == s.end();
		}
	}
	double erand() {
		constexpr uint32_t max = 0xFFFFFFFFu;
		static thread_local std::mt19937* generator = nullptr;
		if (!generator) {
			std::hash<std::thread::id> hasher;
			generator = new std::mt19937(clock() + hasher(std::this_thread::get_id()));
		}
		std::uniform_int_distribution<uint32_t> distribution(0, max);
		return distribution(*generator) / (double(max) + 1.);
	}
	double nrand() {
		double u = 0, v = 0;
		while (u == 0) u = erand();
		while (v == 0) v = erand();
		return std::sqrt(-2.0 * std::log(u))* std::cos(2 * std::_Pi * v);
	}

	template<typename b_str_type>
	inline std::vector<std::basic_string<b_str_type>> split(const std::basic_string<b_str_type>& s, char delim) {
		std::stringstream ss(s);
		std::basic_string<b_str_type> item;
		std::vector<std::basic_string<b_str_type>> elems;
		while (std::getline(ss, item, delim)) 
			elems.push_back(std::move(item));
		return std::move(elems);
	}

	enum class age_type {
		kid = 0, teen = 1, mature = 2, old = 3
		, null = 4
	};
	enum class disease_state {
		healthy = 0, hidden_nonspreading = 1, hidden_spreading = 2, active_spread = 3, spreading_immune = 4, immune = 5, dead = 6
		, null = 7
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

	inline bool state_considered_sick(disease_state dis_st) {
		return (dis_st!= disease_state::dead &&	dis_st != disease_state::healthy &&	dis_st != disease_state::immune);
	}
	inline bool state_considered_stable(disease_state dis_st) {
		return (dis_st != disease_state::healthy && dis_st != disease_state::immune);
	}

	struct disease_progress {
		virtual mpiss::disease_state update_disease_state(
			mpiss::age_type age_t,
			mpiss::disease_state prev_state,
			const int64_t& time_since_contact,
			const double& val
		) const {
			return prev_state;
		};
	};
}

#endif