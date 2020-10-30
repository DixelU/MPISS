#include <iostream>
#include <fstream>

#include "mpiss_town.h"
#include "probabiliy_disease_progress.h"

#include "JSON/JSON.h"
#include "tinyexpr.h"

struct town_info {
	std::wstring info_filename;
	
};

int main() {
	std::ios_base::sync_with_stdio(false); 

	remove("output.csv");
	constexpr size_t bufsize = 1024 * 1024;
	char* buf = new char[bufsize];
	std::ofstream fout("output.csv");
	fout.rdbuf()->pubsetbuf(buf, bufsize);

	auto cem = new mpiss::cemetery();
	constexpr size_t N = 50000;

	constexpr double Tn = 21, Ph = 0.99, Pimm = (Ph / Tn), Pdead_or_imm = ((1.f - Ph) / Tn) + Pimm;
	auto vec = std::vector<mpiss::single_prob_branch>{
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({
			{mpiss::disease_state::immune, Pimm},
			{mpiss::disease_state::dead, Pdead_or_imm},
			{mpiss::disease_state::hidden_nonspreading, 1}
		}),
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({}),
		mpiss::single_prob_branch({})
	};
	auto dp = new mpiss::probability_disease_progress(
		std::vector<decltype(vec)>(mpiss::age_enum_size, vec)
	);
	auto state_spread_modifier = new point<mpiss::state_enum_size>{ 0.0, 1., 1., 1., 1., 0., 0. };

	auto contact_prob = new double(2./Tn);//alpha

	mpiss::room r(contact_prob, state_spread_modifier, cem);
	for (int i = 0; i < N; i++) 
		r.cells.push_back(new mpiss::cell(dp));

	size_t cnt = 0;
	std::string temp = "";
	const std::string delim = ";";

	constexpr size_t iterations_count = 300, cycle_count = 50;
	constexpr size_t amount_of_sick = 100;
	std::vector<std::tuple<double, double, double, double>> count1(iterations_count, std::tuple<double, double, double, double>(0,0,0,0));
	std::vector<std::tuple<double, double, double, double>> count2(iterations_count, std::tuple<double, double, double, double>(0,0,0,0)); 

	std::vector<decltype(count1)> output(cycle_count, count1);
	
	for (int i = 0; i < amount_of_sick; i++)
		r.cells[i]->next_disease_state = mpiss::disease_state::hidden_nonspreading;

	for (size_t i = 0; i < cycle_count; i++) {
		for (size_t c = 0; c < iterations_count; c++) {
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
			t->value = mpiss::erand();
			t->cur_disease_state = t->next_disease_state = mpiss::disease_state::healthy;
		}
		
		for (int i = 0; i < amount_of_sick; i++)
			r.cells[i]->next_disease_state = mpiss::disease_state::hidden_nonspreading;

		cem->deads.clear();
		std::cout << "Loop " << i << std::endl;
	}
	for (size_t c = 0; c < iterations_count; c++) {
		for (auto& v : output) 
			temp += std::to_string((size_t)(std::get<1>(v[c]))) + ";";
		temp += "\n";
		fout << temp;
		std::cout << temp;
		temp.clear();
		cnt++;
	}
}