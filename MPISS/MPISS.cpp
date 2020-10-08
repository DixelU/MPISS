#include <iostream>
#include <fstream>

#include "mpiss_room.h"
#include "mpiss_disease_progress_line.h"
#include "probabiliy_disease_progress.h"

#define line_entry(__t,__s,__o) std::pair<mpiss::disease_state, mpiss::__prob_line>(__t, mpiss::__prob_line{__s,__o})

#define PROB_BASED 

int main() {
	std::ios_base::sync_with_stdio(false); 

	remove("output.csv");
	constexpr size_t bufsize = 1024 * 1024;
	char* buf = new char[bufsize];
	std::ofstream fout("output.csv");
	fout.rdbuf()->pubsetbuf(buf, bufsize);

	auto cem = new mpiss::cemetery();
	constexpr size_t N = 50000;

#ifndef PROB_BASED
	auto vec = std::vector<std::pair<mpiss::disease_state, mpiss::__prob_line>>{
		line_entry(mpiss::disease_state::hidden_nonspreading, 0.5, 0),
		line_entry(mpiss::disease_state::hidden_spreading, 100, 150),
		line_entry(mpiss::disease_state::active_spread, 200, 300),
		line_entry(mpiss::disease_state::dead, 100, 600),
		line_entry(mpiss::disease_state::immune, 99.75, 600),
		line_entry(mpiss::disease_state::healthy, 600, 2000),
	};
	auto dp = new mpiss::aged_disease_progress_line{
		std::vector<mpiss::disease_progress_line>(mpiss::age_enum_size,vec)
	};
	auto state_spread_modifier = new point<mpiss::state_enum_size>{ 0.0,0.1,0.75,1.,0.000000001,0.1 };
	auto contact_prob = new double(0.1);
#else
	constexpr double Tn = 21, Ph = 0.99, Pimm = (Ph / Tn), Pdead = ((1.f - Ph) / Tn) + Pimm;
	auto vec = std::vector<mpiss::single_prob_branch>{
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({
			{mpiss::disease_state::immune, Pimm},
			{mpiss::disease_state::dead, Pdead}
		}),
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({})
	};
	auto dp = new mpiss::probability_disease_progress(
		std::vector<decltype(vec)>(mpiss::age_enum_size, vec)
	);
	auto state_spread_modifier = new point<mpiss::state_enum_size>{ 0.0, 1., 1., 1., 1., 0. };
	auto variable_contact_prob = new double(0.);//variable probability of contact

	auto contact_prob = new double(1./Tn);//alpha

	auto get_contact_prob = [variable_contact_prob,contact_prob, Tn](double fract_of_healthy) -> void {
		*variable_contact_prob = *contact_prob * (fract_of_healthy) / Tn; // // alpha * healthy / N / Th // //
	};

	//*state_spread_modifier *= (1 - Pdead);
#endif

	mpiss::room r(contact_prob, state_spread_modifier, cem);
	for (int i = 0; i < N; i++) 
		r.cells.push_back(new mpiss::cell(dp));

	size_t cnt = 0;
	std::string temp = "";
	const std::string delim = ";";

	constexpr size_t iterations_count = 300, cycle_count = 15;
	constexpr size_t amount_of_sick = 100;
	std::vector<std::tuple<double, double, double, double>> count1(iterations_count, std::tuple<double, double, double, double>(0,0,0,0));
	std::vector<std::tuple<double, double, double, double>> count2(iterations_count, std::tuple<double, double, double, double>(0,0,0,0));

	std::tuple<double, std::string, int, double> c;

	std::vector<decltype(count1)> output(cycle_count, count1);
	
	for (int i = 0; i < amount_of_sick; i++)
		r.cells[i]->next_disease_state = mpiss::disease_state::hidden_nonspreading;

	for (size_t i = 0; i < cycle_count; i++) {
		for (size_t c = 0; c < iterations_count; c++) {
			get_contact_prob(r.counters[(size_t)mpiss::disease_state::healthy] / float(N));
			//std::cout << *variable_contact_prob << std::endl;
			r.make_iteration();
			/*
			std::get<0>(count2[c]) += std::pow(r.counters[(size_t)mpiss::disease_state::healthy],2);
			std::get<1>(count2[c]) += std::pow(r.get_sick_count(),2);
			std::get<2>(count2[c]) += std::pow(r.counters[(size_t)mpiss::disease_state::immune],2);
			std::get<3>(count2[c]) += std::pow(cem->deads.size(),2);
			*/

			std::get<0>(output[i][c]) += r.counters[(size_t)mpiss::disease_state::healthy];
			std::get<1>(output[i][c]) += r.get_sick_count();
			std::get<2>(output[i][c]) += r.counters[(size_t)mpiss::disease_state::immune];
			std::get<3>(output[i][c]) += cem->deads.size();

			/*
			std::get<0>(count1[c]) += r.counters[(size_t)mpiss::disease_state::healthy];
			std::get<1>(count1[c]) += r.get_sick_count();
			std::get<2>(count1[c]) += r.counters[(size_t)mpiss::disease_state::immune];
			std::get<3>(count1[c]) += cem->deads.size();
			*/
			//std::cout << "Iteration " << c << std::endl;
		}
		for (auto& t : cem->deads) 
			r.cells.push_back(t);
		//std::cout << "Deads reset" << std::endl;
		for (auto& t : r.cells) {
			t->time_since_contact = -1;
			//t->value = mpiss::erand();
			t->cur_disease_state = t->next_disease_state = mpiss::disease_state::healthy;
		}
		
		for (int i = 0; i < amount_of_sick; i++)
			r.cells[i]->next_disease_state = mpiss::disease_state::hidden_nonspreading;

		cem->deads.clear();
		std::cout << "Loop " << i << std::endl;
	}
	for (size_t c = 0; c < iterations_count; c++) {
		
		for (auto& v : output) 
			temp += std::to_string((size_t)(std::get<1>(v[c]))/(1)) + ";";
		temp += "\n";
		fout << temp;
		std::cout << temp;
		temp.clear();
		cnt++;
	}
}