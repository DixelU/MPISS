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
	auto vec = std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
				line_entry(mpiss::disease_state::hidden_nonspreading, 0.5, 0),
				line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
				line_entry(mpiss::disease_state::active_spread, 200, 300),
				line_entry(mpiss::disease_state::dead, 100, 400),
				line_entry(mpiss::disease_state::immune, 99.75, 400),
				line_entry(mpiss::disease_state::healthy, 600, 1000),
	};
	auto adpl = new mpiss::aged_disease_progress_line{
		std::vector<mpiss::disease_progress_line>(4,vec)
	};

	auto state_spread_modifier = new point<mpiss::state_enum_size>{ 0.,0.1,0.7,0.9,0.000000001,0.1 };
	auto spread_matrix = new sq_matrix<mpiss::age_enum_size>{
		{0.9,0.9,0.9,0.9},
		{0.9,0.9,0.9,0.9},
		{0.9,0.9,0.9,0.9},
		{0.9,0.9,0.9,0.9}
	};
	auto percent = new double(0.03);

	mpiss::room r(percent,spread_matrix, state_spread_modifier,cem);
	for (int i = 0; i < 500000; i++) {
		r.cells.push_back(new mpiss::cell(adpl));
	}
	r.cells[65488]->cur_disease_state = mpiss::disease_state::hidden_nonspreading;
	size_t cnt = 0;
	while (true) {
		r.make_iteration();
		std::cout << cnt << ": " << r.cells.size() << " " << cem->deads.size() << std::endl;
		r.print_counters(std::cout);
 		cnt++;
	}
}
