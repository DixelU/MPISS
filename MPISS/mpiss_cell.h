#pragma once
#ifndef MPISS_CELL_NDEF
#define MPISS_CELL_NDEF

#include "mpiss_header.h"

namespace mpiss {
	struct cell {
		mpiss::age_type age_t;
		disease_state cur_disease_state;
		disease_state next_disease_state;
		int64_t time_since_contact;
		disease_progress* dp_module;
		double value;
		cell(disease_progress* dp_module, mpiss::age_type age_t = mpiss::age_type::mature) :
			age_t(age_t), 
			cur_disease_state(mpiss::disease_state::healthy), 
			next_disease_state(mpiss::disease_state::healthy), 
			time_since_contact(-1), 
			dp_module(dp_module) {
			value = erand();
		}
		inline virtual void reset() {
			cur_disease_state = mpiss::disease_state::healthy;
			next_disease_state = mpiss::disease_state::healthy;
			time_since_contact = -1;
			value = erand();
		}
		virtual void make_iteration() {
			if (cur_disease_state == mpiss::disease_state::dead)
				return;
			bool is = (cur_disease_state != disease_state::healthy);
			time_since_contact += is;
			auto new_state = dp_module->update_disease_state(age_t, cur_disease_state, time_since_contact, value);
			if (new_state != cur_disease_state) {
				value = erand();
				if (new_state == mpiss::disease_state::healthy)
					time_since_contact = -1;
				next_disease_state = new_state;
			}
		}
		void set_next_iter_state() {
			cur_disease_state = next_disease_state;
		}
	};
}
#endif 