#include <iostream>
#include <iostream>

#include "mpiss_objects.h"

#define adpl_constr(E) mpiss::aged_disease_progress_line{}
#define adpl_even_entry(E) std::vector<mpiss::disease_progress_line>{E,E,E,E}
#define dpl_entry(E) std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{E}
#define line_entry(__t,__s,__o) std::pair<mpiss::disease_state, mpiss::__prob_line>(__t, mpiss::__prob_line{__s,__o})

int main() {
	auto cem = new mpiss::cemetery();
	auto prob = new double(0.03);
	auto adpl = new mpiss::aged_disease_progress_line{
		std::vector<mpiss::disease_progress_line>{
			std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
				line_entry(mpiss::disease_state::hidden_nonspreading, 50, 100),
				line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
				line_entry(mpiss::disease_state::active_spread, 200, 300),
				line_entry(mpiss::disease_state::dead, 300, 400),
				line_entry(mpiss::disease_state::immune, 315, 400),
				line_entry(mpiss::disease_state::healthy, 600, 1000),
			},std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
				line_entry(mpiss::disease_state::hidden_nonspreading, 50, 100),
				line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
				line_entry(mpiss::disease_state::active_spread, 200, 300),
				line_entry(mpiss::disease_state::dead, 300, 400),
				line_entry(mpiss::disease_state::immune, 315, 400),
				line_entry(mpiss::disease_state::healthy, 600, 1000),
			},std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
				line_entry(mpiss::disease_state::hidden_nonspreading, 50, 100),
				line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
				line_entry(mpiss::disease_state::active_spread, 200, 300),
				line_entry(mpiss::disease_state::dead, 300, 400),
				line_entry(mpiss::disease_state::immune, 315, 400),
				line_entry(mpiss::disease_state::healthy, 600, 1000),
			},std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
				line_entry(mpiss::disease_state::hidden_nonspreading, 50, 100),
				line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
				line_entry(mpiss::disease_state::active_spread, 200, 300),
				line_entry(mpiss::disease_state::dead, 300, 400),
				line_entry(mpiss::disease_state::immune, 315, 400),
				line_entry(mpiss::disease_state::healthy, 600, 1000),
			}
		}
	};

	auto state_spread_modifier = new point<mpiss::state_enum_size>{ 0.,0.01,0.2,0.9,0.000000001,0.1 };
	auto spread_matrix = new sq_matrix<mpiss::age_enum_size>{
		{0.1,0.1,0.1,0.1},
		{0.1,0.1,0.1,0.1},
		{0.1,0.1,0.1,0.1},
		{0.1,0.1,0.1,0.1}
	};
	auto percent = new double(0.03);

	mpiss::room r(percent,spread_matrix, state_spread_modifier,cem);
	for (int i = 0; i < 10000; i++) {
		r.cells.push_back(new mpiss::cell(adpl));
	}
	r.cells.back()->cur_disease_state = mpiss::disease_state::hidden_nonspreading;
	while (true) {
		r.make_iteration();
		std::cout << r.cells.size() << " " << cem->deads.size() << " " << r.cur_ill_count << std::endl;
	}
}
