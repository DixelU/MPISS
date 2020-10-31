#include <iostream>
#include <fstream>
#include <sstream>

#include "mpiss_town.h"
#include "probabiliy_disease_progress.h"

#include "JSON/JSON.h"
#include "exprtk_wrapper.h"

using mpiss::__utils::is_number;

struct town_builder {
	std::wstring info_filename = L"";
	std::string last_error = "";
	std::string warning_list = "";
	size_t population[mpiss::age_enum_size];
	size_t rooms_count[mpiss::shedule_enum_size];

	std::map<mpiss::age_type, std::vector<std::pair<double, mpiss::dynamic_shedule_list>>> main_tracks;

	bool load() {
		std::ifstream in(info_filename);
		std::wstringstream sstream;
		sstream << in.rdbuf();
		std::wstring json = sstream.str();
		last_error = warning_list = "";
		auto JObject = JSON::Parse(json.c_str());
		if (JObject->IsObject()) {
			auto main_obj = JObject->AsObject();
			auto aged_pop_it = main_obj.find(L"aged_population");
			auto rooms_cnt_it = main_obj.find(L"rooms_count");
			if (aged_pop_it == main_obj.end() || rooms_cnt_it == main_obj.end())
				return last_error = "Loading failed: no population/rooms data.", false;
			if (!aged_pop_it->second->IsObject() || !rooms_cnt_it->second->IsObject())
				return last_error = "Loading failed: incorrect type of population/rooms data.", false;
			auto aged_pop_obj = aged_pop_it->second->AsObject();
			auto rooms_cnt_obj = rooms_cnt_it->second->AsObject();
			for (int cur_age = 0; cur_age < mpiss::age_enum_size; cur_age++) {
				std::string enum_name = magic_enum::enum_name<mpiss::age_type>((mpiss::age_type)cur_age).data();
				std::wstring l_enum_name = std::wstring(enum_name.begin(), enum_name.end());
				auto it = aged_pop_obj.find(l_enum_name);
				if (it != aged_pop_obj.end()) {
					if (it->second->IsNumber())
						population[cur_age] = it->second->AsNumber();
					else {
						warning_list += "Not a numeric data for " + enum_name + "s count\n";
						population[cur_age] = 0;
					}
				}
				else {
					warning_list += "No explicit data for amount of " + enum_name + "s count\n";
					population[cur_age] = 0;
				}
			}
			for (int cur_shedule = 0; cur_shedule < mpiss::shedule_enum_size; cur_shedule++) {
				std::string enum_name = magic_enum::enum_name<mpiss::shedule_place>((mpiss::shedule_place)cur_shedule).data();
				std::wstring l_enum_name = std::wstring(enum_name.begin(), enum_name.end());
				auto it = rooms_cnt_obj.find(l_enum_name);
				if (it != rooms_cnt_obj.end()) {
					if (it->second->IsNumber())
						rooms_count[cur_shedule] = it->second->AsNumber();
					else {
						warning_list += "Not a numeric data for " + enum_name + "s count\n";
						rooms_count[cur_shedule] = 0;
					}
				}
				else {
					warning_list += "No explicit data for amount of " + enum_name + "s count\n";
					rooms_count[cur_shedule] = 0;
				}
			}
			auto possible_tracks_it = main_obj.find(L"possible_main_tracks");
			auto additional_tracks_it = main_obj.find(L"additional_tracks");

			if (possible_tracks_it == main_obj.end())
				warning_list += "No main tracks data.\n";
			else if (!possible_tracks_it->second->IsObject())
				warning_list += "Main tracks data object is not an object.\n";
			else {
				auto possible_tracks = possible_tracks_it->second->AsObject();
				for (int cur_age = 0; cur_age < mpiss::age_enum_size; cur_age++) {
					std::string enum_name = magic_enum::enum_name<mpiss::age_type>((mpiss::age_type)cur_age).data();
					std::wstring l_enum_name = std::wstring(enum_name.begin(), enum_name.end());
					auto it = possible_tracks.find(l_enum_name);
					if (it != possible_tracks.end()) {
						if (it->second->IsObject()) {
							auto possible_tracks_for_cur_age = it->second->AsObject();
							for (auto& key : possible_tracks_for_cur_age) {
								if (!is_number(key.first)) {
									warning_list += "Track data contains non-numered track in " + enum_name + "\n";
									continue;
								}
								else {
									auto key_name = std::to_string(std::stol(key.first));
									if (!key.second->IsObject()) {
										warning_list += "Track data is not an object in " + enum_name + ":" + key_name + "\n";
										continue;
									}
									auto track_data = key.second->AsObject();
									auto prob_it = track_data.find(L"prob");
									auto track_it = track_data.find(L"track");
									auto length_it = track_data.find(L"length");
									auto pos_rand_it = track_data.find(L"pos_rand");
									if (prob_it == track_data.end() ||
										track_it == track_data.end() ||
										length_it == track_data.end() ||
										pos_rand_it == track_data.end()) {
										warning_list += "Missing one or more track data fields at " + enum_name + ":" + key_name + "\n";
										continue;
									}

									std::vector<mpiss::shedule_ticket> extracted_shedule_data;
									double prob = 0;

									if (prob_it->second->IsNumber())
										prob = prob_it->second->AsNumber();
									else
										warning_list += "Implicit probability (\"prob\" field) cast to zero was performed (type mismatch at " + enum_name + ":" + key_name + ")\n";
									if (track_it->second->IsString()) {
										auto track_string = track_it->second->AsString();
										for (auto& ch : track_string) {
											mpiss::shedule_place sh_place = mpiss::shedule_place::null;
											switch ((char)ch) {
											case 'H': sh_place = mpiss::shedule_place::home; break;
											case 'W': sh_place = mpiss::shedule_place::work; break;
											case 'T': sh_place = mpiss::shedule_place::transport; break;
											case 'M': sh_place = mpiss::shedule_place::magazine; break;
											case 'S': sh_place = mpiss::shedule_place::school; break;
											}
											if (sh_place == mpiss::shedule_place::null)
												continue;
											extracted_shedule_data.push_back(mpiss::shedule_ticket(sh_place, 0, 0, 0));
										}
										if (track_string.size() && extracted_shedule_data.empty()) {
											warning_list += "No correct track data at " + enum_name + ":" + key_name + "\n";
											continue;
										}
										if (!(length_it->second->IsArray() && pos_rand_it->second->IsArray())) {
											warning_list += "Length/Pos rand data type is not an array at " + enum_name + ":" + key_name + "\n";
											continue;
										}
										auto length_array = length_it->second->AsArray();
										auto pos_rand_array = length_it->second->AsArray();
										if (length_array.size() != pos_rand_array.size() || length_array.size() != extracted_shedule_data.size()) {
											warning_list += "Size mismatch of \"track\", \"length\" and \"pos_rand\" at " + enum_name + ":" + key_name + "\n";
											continue;
										}
										for (size_t i = 0; i < extracted_shedule_data.size(); i++) {
											if (!length_array[i]->IsNumber() || !pos_rand_array[i]->IsNumber()) {
												warning_list += "\"length\" or \"pos_rand\" contain non-numeric values in the arrays at " + enum_name + ":" + key_name + "\n";
												continue;
											}
											extracted_shedule_data[i].len = length_array[i]->AsNumber();
											extracted_shedule_data[i].n_disp = pos_rand_array[i]->AsNumber();
										}

										auto main_tracks_it = main_tracks.find((mpiss::age_type)cur_age);
										if (main_tracks_it == main_tracks.end())
											main_tracks[(mpiss::age_type)cur_age] = std::vector<std::pair<double, mpiss::dynamic_shedule_list>>();
										main_tracks_it->second.push_back({ prob,mpiss::dynamic_shedule_list(std::vector<std::vector<mpiss::shedule_ticket>>(1,extracted_shedule_data)) });
									}
									else
										warning_list += "Track is not a string (type mismatch at " + enum_name + ":" + key_name + ")\n";

								}
							}
						}
						else 
							warning_list += "Track data object is not an object at: " + enum_name + "\n";
					}
					else 
						warning_list += "No explicit track data for  " + enum_name + "s\n";
				}
			}

			if (additional_tracks_it != main_obj.end() && !additional_tracks_it->second->IsObject())
				warning_list += "Additional tracks are present, yet are not an object.\n";
			else {
				auto additional_tracks = additional_tracks_it->second->AsObject();
			}
		}
		else
			return last_error = "Loading failed: not an object.", false;
	}
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