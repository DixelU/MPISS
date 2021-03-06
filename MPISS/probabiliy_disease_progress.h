#pragma once
#ifndef MPISS_PDP_NDEF
#define MPISS_PDP_NDEF

#include "mpiss_header.h"

namespace mpiss {
	struct single_prob_branch {
		std::vector<std::pair<mpiss::disease_state, double>> branches;
		single_prob_branch(const std::vector<std::pair<mpiss::disease_state, double>>& branches):
			branches(branches){}
		inline mpiss::disease_state evalute_prob(double rnd, mpiss::disease_state cur_state) const {
			if (branches.empty())
				return cur_state;
			auto prob = 0.;
			for (const auto& [state, probability] : branches) {
				prob += probability;
				if (rnd < prob)
					return state;
			}
			return cur_state;
		}
	};

	struct probability_disease_progress :disease_progress {
		std::vector<std::vector<single_prob_branch>> data;
		probability_disease_progress(const std::vector<std::vector<single_prob_branch>>& d) :data(d) {}
		inline mpiss::disease_state update_disease_state(
			mpiss::age_type age_t,
			mpiss::disease_state prev_state,
			const int64_t& time_since_contact,
			const double& val
		) const override {
			return data[(size_t)age_t][(size_t)prev_state].evalute_prob(mpiss::erand(), prev_state);
		}
	};
}

#endif 