#pragma once
#ifndef MPISS_SHEDULED_CELL_NDEF
#define MPISS_SHEDULED_CELL_NDEF

#include "mpiss_room.h"

namespace mpiss {
	enum class shedule_place {
		home, transportation, work, magazine, school
	};
	struct shedule_ticket {
		shedule_place type;
		size_t id, len, left;
		double n_disp;
		shedule_ticket(shedule_place type, size_t id, size_t len, double n_disp) :
			type(type), id(id), len(len), n_disp(n_disp) {
			left = len;
		}
		inline double get_rand_id() {
			return id + nrand() * n_disp;
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
	struct sheduled_cell : cell {
		std::vector<shedule_ticket> *shedule;
		int cur_shedule;
		int prev_shedule;
		sheduled_cell(disease_progress* dp_module, std::vector<shedule_ticket> *shedule, mpiss::age_type age_t = mpiss::age_type::mature) :
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