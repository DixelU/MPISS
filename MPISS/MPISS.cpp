﻿#define NOMINMAX 1
#include <Windows.h>
#include <algorithm>

#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include <thread>
#include <locale>

#include "mpiss_town.h"
#include "probabiliy_disease_progress.h"
#include "pooled_thread.h"

#include "JSON/JSON.cpp"
#include "JSON/JSONValue.cpp"

#include "exprtk_wrapper.h"

#include "matrix.h"

mpiss::probability_disease_progress regular_progress_builder(){
	mpiss::single_prob_branch f_h({});
	mpiss::single_prob_branch f_hns({ //hidden_nonspreading
		{mpiss::disease_state::hidden_spreading, mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_hs({//hidden_spreading
		{mpiss::disease_state::active_spread,  mpiss::erand() * 0.1},
		});
	mpiss::single_prob_branch f_as({//active_spread
		{mpiss::disease_state::spreading_immune,  mpiss::erand() * 0.1},
		});
	mpiss::single_prob_branch f_si({//spreading_immune
		{mpiss::disease_state::immune,  mpiss::erand() * 0.1},
		{mpiss::disease_state::dead,  mpiss::erand() * 0.1}
		});
	mpiss::single_prob_branch f_i({//immune
		{mpiss::disease_state::healthy,  mpiss::erand() * 0.1}
		});
	auto vec = std::vector<mpiss::single_prob_branch>{ f_h, f_hns, f_hs, f_as, f_si, f_i, f_h };
	return mpiss::probability_disease_progress(
		std::vector<decltype(vec)>(mpiss::age_enum_size, vec)
	);
}

inline std::vector<double*> get_all_parameters(
	mpiss::probability_disease_progress& pdp,
	point<mpiss::state_enum_size>& state_spread_modifier,
	point<mpiss::shedule_enum_size>& contact_probabilities
) {
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
	for (auto& prob : contact_probabilities.pt)
		vec.push_back(&prob);
	return vec;
}

inline matrix load_params(const std::vector<double*>& params) {
	matrix mx;
	mx.resize(params.size(), 1);
	for (int i = 0; i < params.size(); i++)
		mx.at(0, i) = *params[i];
	return mx;
}

inline void unload_params(std::vector<double*>& params, matrix& mx) {
	for (int i = 0; i < params.size(); i++)
		*params[i] = mx.at(0, i);
}

struct params_manipulator {
	inline static matrix gradient(std::function<double(const matrix&)> f, const matrix& p) {
		constexpr double step = 0.000001;
		matrix e(p.rows(), p.cols()), D(p.rows(), p.cols());
		auto h = p * step;
		h.sapply([step](double& v) {if (v < DBL_EPSILON) v = step; });
		for (size_t i = 0; i < e.rows(); i++) {
			e.at(0, i) = h.at(0, i);
			auto pph = p + e, pmh = p - e;
			pph.sapply([](double& v) { v = std::clamp(v, 0., 1.); });
			pmh.sapply([](double& v) { v = std::clamp(v, 0., 1.); });
			D.at(0, i) = (f(pph) - f(pmh)) / (pph.at(0, i) - pmh.at(0, i));
			e.at(0, i) = 0;
		}
		return D;
	}
	inline static std::pair<double, double> find_edge_multipliers(matrix D, const matrix& pt) {
		double top = 1e12, bottom = -1e12;
		for (int i = 0; i < D.rows(); i++) {
			double cur_d = D.at(0, i);
			double cur_p = pt.at(0, i);
			if (top * cur_d + cur_p > 1)
				top = (1 - cur_p) / cur_d;
			if (top * cur_d + cur_p < 0)
				top = -cur_p / cur_d;
			if (bottom * cur_d + cur_p > 1)
				bottom = (1 - cur_p) / cur_d;
			if (bottom * cur_d + cur_p < 0)
				bottom = -cur_p / cur_d;
		}
		return { top,bottom };
	}
	inline static std::tuple<bool, std::optional<matrix>, double, double> align_gradient(const matrix& D, const matrix& pt, double epsilon) {
		auto [top, bottom] = find_edge_multipliers(D, pt);
		if (std::abs(top)+std::abs(bottom) > epsilon)
			return { true, D, top, bottom };
		matrix modifiedD(D.rows(), D.cols());
		for (int i = 0; i < D.rows(); i++) {
			modifiedD.at(0, i) = std::clamp(D.at(0, i) + pt.at(0, i), 0., 1.) - pt.at(0, i);
		}
		if (modifiedD.norma(1.) < epsilon)
			return { false, {}, 0,0 };
		auto [newtop, newbottom] = find_edge_multipliers(modifiedD, pt);
		if (std::abs(newtop) + std::abs(newbottom) < epsilon)
			return { false, {}, 0, 0 };
		return { true, modifiedD, newtop, newbottom };
	}
	inline static double onedim_minimistaion(std::function<double(double)> func, double a, double b, double Eps = 0.001) {
		constexpr double tau = 0.61803398874989484820458683436564;
		double x, y, fx, fy;
		x = a + (1 - tau) * (b - a);
		y = a + tau * (b - a);
		fx = func(x); fy = func(y);
		while (std::abs(b - a) >= Eps) {
			if (fx > fy) {
				a = x;
				x = y;
				fx = fy;
				y = a + tau * (b - a);
				fy = func(y);
			}
			else {
				b = y;
				y = x;
				fy = fx;
				x = a + (1 - tau) * (b - a);
				fx = func(x);
			}
		}
		return 0.5 * (a + b);
	}
	//**  minima and h  **//
	inline static std::pair<double,double> onedim_grid_minima(std::function<double(double)> func, double a, double b, int n = 6, double Eps = 0.001) {
		double minima = a, minfunc = INFINITY;
		double cur, h = (b - a)/n;
		cur = a + h;
		n -= 2;
		while (n-->0) {
			double newfuncvalue = func(cur);
			if (minfunc > newfuncvalue) {
				minfunc = newfuncvalue;
				minima = cur;
			}
			cur += h;
		}
		return { minima , h };
	}
	inline static matrix simple_gradient_meth(std::function<double(const matrix&)> func, matrix begin) {
		constexpr double epsilon = 0.000001;
		matrix prev;
		do {
			auto grad = (gradient(func, begin))*(-1);
			//std::cout << "Orig step: " << grad.transpose() << std::endl;
			auto [is_not_degenerated, optD, top, bottom] = align_gradient(grad, begin, epsilon);
			if (!is_not_degenerated)
				return begin;
			auto D = optD.value();

			//std::cout << "Final step: " << D.transpose() << std::endl;

			auto odfunc = [&](double p) -> double {
				return func(begin + p * D);
			};

			auto [minima, h] = onedim_grid_minima(odfunc, bottom, top, 20, epsilon);
			auto local_minima = onedim_minimistaion(odfunc, minima - h, minima + h, epsilon);
			prev = (begin);
			auto next_step = D * local_minima;
			begin += next_step;

			if (next_step.norma(2) < epsilon)
				break;

			//std::cout << "Step norma: " << next_step.norma(1) << std::endl;
			//if ((prev - begin).norma(1) < epsilon)
				//break;
			//std::cout << "Next approx: " << begin.transpose();
		} while (true);
		return begin;
	}
};

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
				singular_ticket.left = singular_ticket.len = std::stol(ticket_sub_separated_data[2]);

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


struct town_manipulator {
	town_builder t_builder;
	std::vector<pooled_thread*> threads;

	struct thread_data {
		mpiss::town* town;
		std::vector<std::vector<point<mpiss::state_enum_size>>> counters; 
		size_t initial_amount_of_hns;
	};

	std::vector<std::pair<point<mpiss::state_enum_size>, point<mpiss::state_enum_size>>>
		start_simulation_with_current_parameters(const size_t repeats, const size_t iters, const size_t initial_amount_of_hns) {

		size_t amount_of_threads = std::max(1, int32_t(std::thread::hardware_concurrency()) - 1);
		while (threads.size() < amount_of_threads) 
			threads.push_back(new pooled_thread());

		double_t repeats_per_thread = double_t(repeats) / amount_of_threads;
		size_t repeats_per_thread_rounded_up = std::ceil(repeats_per_thread);
		size_t fixed_repeats = repeats_per_thread_rounded_up * amount_of_threads;

		auto automation_func = [&](void** ptr) {
			auto t_ptr = *(thread_data**)ptr;
			for (size_t rep = 0; rep < repeats_per_thread_rounded_up; rep++) {
				mpiss::shedule_place first_not_empty_place = mpiss::shedule_place::null;
				for (size_t i = 0; i < mpiss::shedule_enum_size; i++) {
					if (t_ptr->town->places[(mpiss::shedule_place)i].size()) {
						first_not_empty_place = (mpiss::shedule_place)i;
						break;
					}
				}
				if (first_not_empty_place == mpiss::shedule_place::null)
					throw std::runtime_error("No cells!");
				size_t size = t_ptr->town->places[first_not_empty_place].size();
				for (size_t i = 0; i < t_ptr->initial_amount_of_hns; i++) {
					size_t rounded_rnd;
					do {
						rounded_rnd = std::floor(size * mpiss::erand());
					} while (t_ptr->town->places[first_not_empty_place][rounded_rnd].cells.empty());
					auto& vec = t_ptr->town->places[first_not_empty_place][rounded_rnd].cells;
					double fin_id = std::floor(vec.size() * mpiss::erand());
					if (vec[size_t(fin_id)]->next_disease_state == mpiss::disease_state::hidden_nonspreading) {
						i--;
						continue;
					}
					vec[size_t(fin_id)]->next_disease_state = mpiss::disease_state::hidden_nonspreading;
				}

				for (size_t i = 0; i < iters; i++) {
					t_ptr->town->make_iteration();
					t_ptr->town->update_counters();
					t_ptr->counters[rep][i] = t_ptr->town->counters;
				}
				t_ptr->town->reset();
			}
		};

		for (auto& pthread : threads) {
			auto ptr = ((thread_data**)pthread->__void_ptr_accsess());
			if (!*ptr) {
				*ptr = new thread_data;
			}
			(*ptr)->initial_amount_of_hns = initial_amount_of_hns;
			(*ptr)->town = t_builder.create_town();
			pthread->set_new_function(automation_func);
			(*ptr)->counters.clear();
			(*ptr)->counters.resize(repeats_per_thread_rounded_up, 
				std::vector<point<mpiss::state_enum_size>>(iters, point<mpiss::state_enum_size>()));
			pthread->sign_awaiting();
		}

		bool not_all_are_ready = true;
		while (not_all_are_ready) {
			not_all_are_ready = false;
			for (auto& pthread : threads) {
				if (pthread->get_state() != pooled_thread::state::idle)
					not_all_are_ready = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}

		point<mpiss::state_enum_size> mean, mean_sq, std_err, zero;
		std::vector<std::pair<point<mpiss::state_enum_size>, point<mpiss::state_enum_size>>> result;

		for (size_t i = 0; i < iters; i++) {
			mean = zero;
			mean_sq = zero;
			std_err = zero;
			size_t cnt = 0;
			for (auto& th : threads) {
				auto ptr = *((thread_data**)th->__void_ptr_accsess());
				for (auto& cnt_vec : ptr->counters) {
					cnt++;
					auto cur_point = cnt_vec[i];
					mean += cur_point;
					mean_sq += (cur_point ^ 2);
				}
			}
			mean /= fixed_repeats;
			mean_sq /= fixed_repeats;
			std_err = mean_sq - (mean ^ 2);
			std_err = std_err ^ 0.5;

			result.push_back({ mean, std_err });
		}

		return result;
	}
};

struct comma final : std::numpunct<char> {
	char do_decimal_point() const override { return ','; }
};

int __main() {
	int counter = 0;
	auto func = [&](const matrix& v)->double {
		auto innerfunc = [](double& v)->void {v = (v - 0.789865468); };
		auto mat = v.apply(innerfunc);
		mat.at(0, 0) *= 4;
		auto val = mat.norma();
		//std::cout << "Func at " << v.transpose() << val << std::endl;
		counter++;
		return val;
	};
	matrix begin = { {0.1,0.23,0.17,0.24,0.51} };
	auto end = params_manipulator::simple_gradient_meth(func, begin.transpose());
	std::cout << func(end) << std::endl;
	std::cout << "Function call count: " << counter << std::endl;
	return 0;
}

int main() {
	std::ios_base::sync_with_stdio(false); 
	town_manipulator t_manip;
	decltype(MOFD(L"", L"JSON Files(*.json)\0*.json\0")) files;
	do {
		files = MOFD(L"Get town info file\n", L"JSON Files(*.json)\0*.json\0");
	} while (files.empty());

	t_manip.t_builder.info_filename = files[0];

	if (!t_manip.t_builder.load()) {
		std::cout << t_manip.t_builder.last_error << std::endl;
	}
	std::cout << t_manip.t_builder.warning_list << std::endl;

	auto t_ptr = t_manip.t_builder.create_town();
	
	decltype(MOFD(L"", L"Text Files(*.txt)\0*.txt\0")) params;
	do {
		params = MOFD(L"Get disease parameters\n", L"Text Files(*.txt)\0*.txt\0");
	} while (params.empty());

	std::ifstream fin(params[0]);
	
	auto vals = get_all_parameters(*t_manip.t_builder.pdp, *t_manip.t_builder.state_spread_modifier, *(t_manip.t_builder.contact_probabilities));

	for (auto& ptr : vals) {
		fin >> *ptr;
	}

	size_t reps = 100, iters=1000, initials=1;
	std::cout << "Approximate repeats count: ";
	std::cin >> reps;
	std::cout << "Amount of iterations: ";
	std::cin >> iters;
	std::cout << "Amount of initially ill: ";
	std::cin >> initials;
	std::vector<std::pair<point<mpiss::state_enum_size>, point<mpiss::state_enum_size>>> result;

	result = t_manip.start_simulation_with_current_parameters(reps, iters, initials);

	std::ofstream fout("output.csv");

	if (true)
		fout.imbue(std::locale(std::locale::classic(), new comma));

	for (auto& iter : result) {
		for (size_t i = 0; i < mpiss::state_enum_size; i++) {
			fout << iter.first[i] << ";" << iter.second[i] << ";";
		}
		fout << std::endl;
	}

	fout.close();

	return 0;
}
