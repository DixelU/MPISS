#define NOMINMAX 1
#include <Windows.h>
#include <algorithm>

#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "mpiss_town.h"
#include "probabiliy_disease_progress.h"

#include "JSON/JSON.cpp"
#include "JSON/JSONValue.cpp"

#include "exprtk_wrapper.h"

mpiss::probability_disease_progress regular_progress_builder(){
	mpiss::single_prob_branch f_h({});
	mpiss::single_prob_branch f_hns({
		{mpiss::disease_state::hidden_spreading, mpiss::erand() * 0.1},
		{mpiss::disease_state::spreading_immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::healthy, mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_hs({
		{mpiss::disease_state::active_spread,  mpiss::erand() * 0.1},
		{mpiss::disease_state::spreading_immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::healthy,  mpiss::erand() * 0.1},
		{mpiss::disease_state::dead,  mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_as({
		{mpiss::disease_state::spreading_immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::healthy,  mpiss::erand() * 0.1},
		{mpiss::disease_state::dead,  mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_si({
		{mpiss::disease_state::immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::healthy,  mpiss::erand() * 0.1},
		{mpiss::disease_state::dead,  mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_i({
		{mpiss::disease_state::healthy,  mpiss::erand() * 0.1}
		});
	auto vec = std::vector<mpiss::single_prob_branch>{ f_h,f_hns,f_hs,f_as,f_si,f_i };
	return mpiss::probability_disease_progress(
		std::vector<decltype(vec)>(mpiss::age_enum_size, vec)
	);
}

inline std::vector<double*> get_all_parameters(mpiss::probability_disease_progress& pdp, point<mpiss::state_enum_size>& state_spread_modifier) {
	std::vector<double*> vec;
	for (auto& aged_pdp : pdp.data) {
		for (auto& stated_pdp : aged_pdp) {
			for (auto& branch : stated_pdp.branches) {
				vec.push_back(&branch.second);
			}
		}
	}
	for (auto& modifier : state_spread_modifier.pt) 
		vec.push_back(&modifier);
	return vec;
}

inline std::vector<std::wstring> MOFD(const wchar_t* Title, const wchar_t* Filter) {
	OPENFILENAME ofn;       // common dialog box structure
	wchar_t szFile[50000];       // buffer for file name
	std::vector<std::wstring> InpLinks;
	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(szFile, 50000);
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = Title;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if (GetOpenFileName(&ofn)) {
		std::wstring Link = L"", Gen = L"";
		int i = 0, counter = 0;
		for (; i < 600 && szFile[i] != '\0'; i++) {
			Link.push_back(szFile[i]);
		}
		for (; i < 49998;) {
			counter++;
			Gen = L"";
			for (; i < 49998 && szFile[i] != '\0'; i++) {
				Gen.push_back(szFile[i]);
			}
			i++;
			if (szFile[i] == '\0') {
				if (counter == 1) InpLinks.push_back(Link);
				else InpLinks.push_back(Link + L"\\" + Gen);
				break;
			}
			else {
				if (Gen != L"")InpLinks.push_back(Link + L"\\" + Gen);
			}
		}
		return InpLinks;
	}
	return {};
}
using mpiss::__utils::is_number;

struct town_manipulator {

};

struct town_builder {
	std::wstring info_filename = L"";
	std::string last_error = "";
	std::string warning_list = "";
	size_t population[mpiss::age_enum_size];
	size_t rooms_count[mpiss::shedule_enum_size];

	point<mpiss::shedule_enum_size>* contact_probabilities = new point<mpiss::shedule_enum_size>;

	point<mpiss::state_enum_size>* state_spread_modifier = new point<mpiss::state_enum_size>;
	mpiss::probability_disease_progress* pdp;
	double* time = new double(0);

	std::vector<exprtk_wrapper*> lockdown_func;

	size_t ticks_in_day = 1;
	std::vector<std::pair<mpiss::age_type, mpiss::dynamic_shedule_list>> cells_tracks;

	mpiss::town* built_town;

	template<typename b_str_type>
	inline static auto track_read(std::basic_string<b_str_type> str) {
		std::vector<mpiss::shedule_ticket> shed_ticket;
		mpiss::shedule_ticket singular_ticket;
		auto tickets = mpiss::split<b_str_type>(str,(b_str_type)';');
		for (auto& ticket : tickets) {
			if (ticket.empty())
				continue;

			auto ticket_sub_separated_data = mpiss::split<b_str_type>(ticket, (b_str_type)':');

			if (ticket_sub_separated_data.size()) {
				auto ptr = std::find_if(ticket_sub_separated_data[0].begin(), ticket_sub_separated_data[0].end(), [](const auto& ch) {
					return (ch == (b_str_type)'H') || (ch == (b_str_type)'W') || (ch == (b_str_type)'T') || (ch == (b_str_type)'M') || (ch == (b_str_type)'S');
				});
				if (ptr != ticket_sub_separated_data[0].end()) {
					switch ((char)ticket_sub_separated_data[0][0]) {
					case 'H':
						singular_ticket.type = mpiss::shedule_place::home;
						break;
					case 'W':
						singular_ticket.type = mpiss::shedule_place::work;
						break;
					case 'T':
						singular_ticket.type = mpiss::shedule_place::transport;
						break;
					case 'M':
						singular_ticket.type = mpiss::shedule_place::magazine;
						break;
					case 'S':
						singular_ticket.type = mpiss::shedule_place::school;
						break;
					}
				}
			}
			else
				continue;

			if (ticket_sub_separated_data.size() > 1 && ticket_sub_separated_data[1].size())
				singular_ticket.id = std::stol(ticket_sub_separated_data[1]);

			if (ticket_sub_separated_data.size() > 2 && ticket_sub_separated_data[2].size())
				singular_ticket.len = std::stol(ticket_sub_separated_data[2]);

			if (ticket_sub_separated_data.size() > 3 && ticket_sub_separated_data[3].size())
				singular_ticket.n_disp = std::stod(ticket_sub_separated_data[3]);
			
			shed_ticket.push_back(singular_ticket);
		}
		return std::move(shed_ticket);
	}

	inline static void update_maximal_tickets( 
		const std::vector<mpiss::shedule_ticket>& new_shedule_list, 
		std::array<size_t, mpiss::shedule_enum_size>& total_rooms_used 
	) {
		for (const auto& ticket : new_shedule_list) 
			total_rooms_used[(size_t)ticket.type] = std::max(total_rooms_used[(size_t)ticket.type], ticket.id);
	}

	bool load() {
		std::ifstream in(info_filename, std::ios::in);
		std::array<size_t, mpiss::shedule_enum_size> total_rooms_used;
		std::wstring json;
		std::copy(std::istream_iterator<char>(in), std::istream_iterator<char>(), std::back_inserter(json));
		last_error = warning_list = "";
		for (int i = 0; i < mpiss::age_enum_size; i++)
			population[i] = 0;
		for (int i = 0; i < mpiss::shedule_enum_size; i++)
			rooms_count[i] = total_rooms_used[i] = 0;
		pdp = new mpiss::probability_disease_progress(regular_progress_builder());
		auto JObject = JSON::Parse(json.c_str());
		if (!JObject)
			return last_error = "JObject - is null", false;
		if (JObject->IsObject()) {
			auto& main_obj = JObject->AsObject();
			auto town_cit = main_obj.find(L"town");
			auto lockdown_functions_cit = main_obj.find(L"lockdown_functions");
			auto ticks_in_day_cit = main_obj.find(L"ticks_in_day");

			if(ticks_in_day_cit == main_obj.end() || !ticks_in_day_cit->second->IsNumber())
				return last_error = "ticks_in_day field parse error (missing/wrong type)", false;

			ticks_in_day = ticks_in_day_cit->second->AsNumber();

			if (town_cit == main_obj.end())
				return last_error = "Missing town info", false;
			if(!town_cit->second->IsArray())
								return last_error = "Town info is not an object", false;

			auto& town_array = town_cit->second->AsArray();
			size_t town_id = 0;
			for (auto& obj : town_array) {
				if (!obj->IsObject()) {
					warning_list += "Town info element " + std::to_string(town_id) + " is not an object\n";
					town_id++;
					continue;
				}
				auto& obj_ref = obj->AsObject();
				auto age_cit = obj_ref.find(L"age");
				auto track_cit = obj_ref.find(L"track");
				auto special_track_cit = obj_ref.find(L"special_track");
				
				if (age_cit == obj_ref.end() || track_cit == obj_ref.end() || special_track_cit == obj_ref.end()) {
					warning_list += "Town info fields \"age\", \"track\", \"special_track\" are missing at line " + std::to_string(town_id) + "\n";
					town_id++;
					continue;
				}

				if (!age_cit->second->IsString()) {
					warning_list += "Town info (field \"age\") is not a string at" + std::to_string(town_id) + "\n";
					town_id++;
					continue;
				}

				if (!track_cit->second->IsString()) {
					warning_list += "Town info (field \"track\") is not a string at" + std::to_string(town_id) + "\n";
					town_id++;
					continue;
				}

				if (!special_track_cit->second->IsString()) {
					warning_list += "Town info (field \"special_track\") is not a string at" + std::to_string(town_id) + "\n";
					town_id++;
					continue;
				}

				auto& age_ref = age_cit->second->AsString();
				auto& track_ref = track_cit->second->AsString();
				auto& special_track_ref = special_track_cit->second->AsString();

				auto age_str = std::string(age_ref.begin(), age_ref.end());

				auto age_type = magic_enum::enum_cast<mpiss::age_type>(age_str);
				if (!age_type.has_value()) {
					warning_list += "Town info (field \"age\") contains a non-enum value(" + age_str + ") at " + std::to_string(town_id) + "\n";
					town_id++;
					continue;
				}

				mpiss::dynamic_shedule_list shedule_list({});

				auto reg_shedule_list = track_read(std::string(track_ref.begin(), track_ref.end()));
				auto spec_shedule_list = track_read(std::string(special_track_ref.begin(), special_track_ref.end()));

				update_maximal_tickets(reg_shedule_list, total_rooms_used);
				update_maximal_tickets(spec_shedule_list, total_rooms_used);

				shedule_list.shedules.push_back(reg_shedule_list);
				shedule_list.shedules.push_back(spec_shedule_list);
				
				population[(size_t)age_type.value()]++;

				cells_tracks.push_back({ age_type.value(), shedule_list });
			}

			if(lockdown_functions_cit != main_obj.end() && lockdown_functions_cit->second->IsObject()){
				auto lockdown_functions = lockdown_functions_cit->second->AsObject();
				for(int i=0;i<=mpiss::age_enum_size;i++){
					auto name = magic_enum::enum_name<mpiss::age_type>((mpiss::age_type)i);
					auto value_cit = lockdown_functions.find(std::wstring(name.begin(),name.end()));
					if (i != mpiss::age_enum_size) {
						lockdown_func.push_back(new exprtk_wrapper({ { "t",*time } }));
					}
					if (value_cit == lockdown_functions.end() || value_cit->second->IsString()) {
						if (i != mpiss::age_enum_size) {
							if (lockdown_func[i])
								delete lockdown_func[i];
							lockdown_func[i] = nullptr;
						}
						continue;
					}
					auto str = value_cit->second->AsString();
					if(i!=mpiss::age_enum_size){
						auto ans = lockdown_func[i]->compile(std::string(str.begin(),str.end()));
						if (!ans) {
							auto vec_of_errors = lockdown_func[i]->get_errors();
							for (auto& [err_line, err_col, err_name, err_desc] : vec_of_errors) {
								warning_list += "Error while compilation of " +
									std::string(str.begin(), str.end()) +
									" lockdown functions at " +
									std::to_string(err_line) + ":" +
									std::to_string(err_col) + " " +
									err_name + ": " + err_desc + "\n";
							}
							if (i != mpiss::age_enum_size) {
								if (lockdown_func[i])
									delete lockdown_func[i];
								lockdown_func[i] = nullptr;
							}
						}
					}
					else {
						for (int j = 0; j < mpiss::age_enum_size; j++) {
							auto ans = lockdown_func[i]->compile(std::string(str.begin(), str.end()));
							if (!ans) {
								auto vec_of_errors = lockdown_func[i]->get_errors();
								for (auto& [err_line, err_col, err_name, err_desc] : vec_of_errors) {
									warning_list += "Error while compilation of " +
										std::string(str.begin(), str.end()) +
										" lockdown functions at " +
										std::to_string(err_line) + ":" +
										std::to_string(err_col) + " " +
										err_name + ": " + err_desc + "\n";
								}
								if (i != mpiss::age_enum_size) {
									if (lockdown_func[i])
										delete lockdown_func[i];
									lockdown_func[i] = nullptr;
								}
							}
						}
					}
				}
			}
			else
				warning_list += "No lockdown data? That town is doomed then...\n";
		}
		else
			return last_error = "Loading failed: not an object.", false;

		for (int i = 0; i < mpiss::shedule_enum_size; i++)
			rooms_count[i] = total_rooms_used[i];
		return true;
	}

	mpiss::town* create_town() {
		mpiss::cemetery* cemetury = new mpiss::cemetery();
		std::unordered_map<mpiss::shedule_place, std::vector<mpiss::room>> places;
		for (auto& [age, shedules] : cells_tracks) {
			auto& first_shedule = shedules.operator*().front();
			for (auto& track : shedules.shedules) {
				for (auto& shedule : track) {
					auto first_place = places.find(shedule.type);
					if (first_place == places.end()) {
						places[shedule.type] = std::vector<mpiss::room>();
						first_place = places.find(shedule.type);
					}
					if (shedule.id >= first_place->second.size())
						first_place->second.resize(
							shedule.id + 1,
							mpiss::room(
								&((*contact_probabilities)[(size_t)shedule.type]), state_spread_modifier, cemetury)
						);
				}
			}
			auto& first_place = places[first_shedule.type];
			first_place[first_shedule.id].cells.push_back(new mpiss::sheduled_cell(pdp, shedules, age));
		}
		return new mpiss::town(cemetury, places);
	}
};

int main() {
	std::ios_base::sync_with_stdio(false); 
	town_builder t_builder;
	decltype(MOFD(L"", L"JSON Files(*.json)\0*.json\0")) files;
	do {
		files = MOFD(L"Get town info file\n", L"JSON Files(*.json)\0*.json\0");
	} while (files.empty());

	t_builder.info_filename = files[0];

	if (!t_builder.load()) {
		std::cout << t_builder.last_error << std::endl;
	}
	std::cout << t_builder.warning_list << std::endl;

	auto ptr = t_builder.create_town();
	
	decltype(MOFD(L"", L"Text Files(*.txt)\0*.txt\0")) params;
	do {
		params = MOFD(L"Get disease parameters\n");
	} while (params.empty());

	std::ifstream fin(params[0]);

	auto vals = get_all_parameters(*t_builder.pdp, *t_builder.state_spread_modifier);

	for (auto ptr : vals) {
		fin >> *ptr;
	}

	return 0;
}
