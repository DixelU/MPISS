#pragma once
#ifndef MPISS_SHEDULED_CELL_NDEF
#define MPISS_SHEDULED_CELL_NDEF

#include "mpiss_room.h"

namespace mpiss {
	enum class shedule_place {
		home = 0, transport = 1, work = 2, magazine = 3, school = 4, null = 5
	};
	constexpr size_t shedule_enum_size = (size_t)shedule_place::null;
	struct shedule_ticket {
		shedule_place type;
		size_t id, len, left;
		double n_disp;
		shedule_ticket(shedule_place type, size_t id, size_t len, double n_disp) :
			type(type), id(id), len(len), n_disp(n_disp) {
			left = len;
		}
		inline double get_rand_id() {
			return id + (nrand() * n_disp);
		}
		inline bool iterate() {
			if (!left) {
				left = len;
				return false;
			}
			left--;
			return true;
		}
	};
	struct dynamic_shedule_list {
		std::vector<std::vector<shedule_ticket>> shedules;
		size_t base_shedule_list_no;
		size_t override_shedule_no;
		dynamic_shedule_list(const std::vector<std::vector<shedule_ticket>>& shedules) : shedules(shedules), base_shedule_list_no(0), override_shedule_no(0) { }
		inline std::vector<shedule_ticket>& operator*() {
			return shedules[override_shedule_no % shedules.size()];
		}
		inline const std::vector<shedule_ticket>& operator*() const {
			return shedules[override_shedule_no % shedules.size()];
		}
		inline void override_shedule_no(size_t override_no) {
			override_shedule_no = override_no;
		}
		inline void revert_all_overrides() {
			override_shedule_no = base_shedule_list_no;
		}
	};
	struct sheduled_cell : cell {
		dynamic_shedule_list& shedule;
		int cur_shedule;
		int prev_shedule;
		sheduled_cell(disease_progress* dp_module, dynamic_shedule_list& shedule, mpiss::age_type age_t = mpiss::age_type::mature) :
			cell(dp_module, age_t), shedule(shedule), cur_shedule(0), prev_shedule(0) {

		}
		inline void reset() override {
			cell::reset();
			(*shedule)[cur_shedule].left = (*shedule)[cur_shedule].len;
			cur_shedule = 0;
		}
		void make_iteration() override {
			cell::make_iteration();
			prev_shedule = cur_shedule;
			if (!(*shedule)[cur_shedule].iterate()) {
				cur_shedule++;
				if (cur_shedule == (*shedule).size())
					cur_shedule = 0;
			}
		}
	};
}

#endif