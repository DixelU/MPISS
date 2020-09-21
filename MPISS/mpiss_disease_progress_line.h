#pragma once

#ifndef MPISS_DPL_NDEF
#define MPISS_DPL_NDEF

#include "mpiss_header.h"

namespace mpiss {
	struct __prob_line {
		const double s, o;
		__prob_line(double s, double o) :s(s), o(o) {}
		double eval(const double& x) const {
			return (-1 / s) * (x - o);
		}
	};

	struct disease_progress_line {
		std::vector<std::pair<mpiss::disease_state, __prob_line>> data;
		disease_progress_line(const std::vector<std::pair<mpiss::disease_state, __prob_line>>& d = std::vector<std::pair<mpiss::disease_state, __prob_line>>()) : data(d) {}
		mpiss::disease_state update_disease_state(const int64_t& time_since_contact, const float& val) const {
			if (val == neg_inf)
				return mpiss::disease_state::dead;
			mpiss::disease_state prev_state = mpiss::disease_state::healthy;
			for (const auto& pr : data) {
				float i_val = pr.second.eval(time_since_contact);
				if (i_val > val)
					return prev_state;
				prev_state = pr.first;
			}
			return prev_state;
		}
	};

	struct aged_disease_progress_line: disease_progress {
		std::vector<disease_progress_line> data;
		aged_disease_progress_line(const std::vector<disease_progress_line>& d = std::vector<disease_progress_line>()) :data(d) {}
		mpiss::disease_state update_disease_state(
			mpiss::age_type age_t, 
			mpiss::disease_state prev_state, 
			const int64_t& time_since_contact,
			const float& val
		) const override {
			return data[(size_t)age_t].update_disease_state(time_since_contact, val);
		}
	};
};

#endif