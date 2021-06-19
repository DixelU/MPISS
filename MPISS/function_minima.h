#pragma once
#ifndef FUNC_MINIMA_DEF_GUARD
#define FUNC_MINIMA_DEF_GUARD

#include <optional>
#include <functional>
#include "matrix.h"
#include "mpiss_header.h"
#include "access_method_data.h"

namespace params_manipulator_globals {
	double desired_range = 1e-8;
	int begin_evolution_sizes = 200;
	int skip_counter = 0;
};

struct params_manipulator {

	inline static matrix gradient(std::function<double(const matrix&)> f, const matrix& p) {
		constexpr double step = 1e-5;
		matrix e(p.rows(), p.cols()), D(p.rows(), p.cols());
		auto h = p * (step / p.norma());
		h.selfapply([step](double& v) {if (v < DBL_EPSILON || isnan(v)) v = step; });
		for (size_t x = 0; x < p.cols(); x++) {
			for (size_t y = 0; y < p.rows(); y++) {
				e.at(x, y) = h.at(x, y);
				auto pph = p + e, pmh = p - e;
				D.at(x, y) = (f(pph) - f(pmh)) / (pph.at(x, y) - pmh.at(x, y));
				e.at(x, y) = 0;
			}
		}
		return D;
	}

	inline static std::tuple<double, matrix, matrix> func_gradient_and_hessian(std::function<double(const matrix&)> f, matrix p) {
		constexpr double step = 1e-5;
		if (p.rows() != 1 && p.cols() != 1)
			throw std::runtime_error("matrix arguments are not supported!");
		if (p.cols() > 1)
			p = p.transpose();
		const size_t size = p.rows();
		matrix e(size, 1), D(size, 1), D2(size, size);
		matrix fppmh(size, 2);
		double fp = f(p);
		auto h = p * (step / p.norma());
		h.selfapply([step](double& v) {if (v < step || isnan(v)) v = step; });

		for (size_t i = 0; i < size; i++) {
			e.at(0, i) = h.at(0, i);
			auto pph = p + e, pmh = p - e;
			e.at(0, i) = 0;
			fppmh.at(0, i) = f(pmh);
			fppmh.at(1, i) = f(pph);
		}

		for (size_t i = 0; i < size; i++) {
			D.at(0, i) = 0.5 * (fppmh.at(1, i) - fppmh.at(0, i)) / h.at(0, i);
			D2.at(i, i) = (fppmh.at(1, i) + fppmh.at(0, i) - 2. * fp) / std::pow(h.at(0, i), 2.);
		}

		for (size_t y = 0; y < size; y++) {
			for (size_t x = 0; x < y; x++) {
				double h1, h2;
				e.at(0, x) = (h1 = h.at(0, x));
				e.at(0, y) = (h2 = h.at(0, y));
				auto pph = p + e, pmh = p - e;
				e.at(0, x) = 0;
				e.at(0, y) = 0;
				double fpph = f(pph);
				double fpmh = f(pmh);
				D2.at(x, y) = (fpph + fpmh + 2. * fp - (fppmh.at(0, x) + fppmh.at(0, y) + fppmh.at(1, x) + fppmh.at(1, y))) / (h.at(0, x) * h.at(0, y) * 2.);
				D2.at(y, x) = D2.at(x, y);
			}
		}
		return { fp, D, D2 };
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
		if (std::abs(top) + std::abs(bottom) > epsilon)
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
	inline static std::pair<double, double> onedim_minimistaion(std::function<double(double)> func, double a, double b, double Eps = 0.001) {
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
		return { 0.5 * (a + b), 0.5 * (fx + fy) };
	}
	//**  minima and h  **//
	inline static std::pair<double, double> onedim_grid_minima(std::function<double(double)> func, double a, double b, int n = 6, double Eps = 0.001) {
		double minima = a, minfunc = INFINITY;
		double cur, h = (b - a) / n;
		cur = a + h;
		n -= 2;
		while (n-- > 0) {
			double newfuncvalue = func(cur);
			if (minfunc > newfuncvalue) {
				minfunc = newfuncvalue;
				minima = cur;
			}
			cur += h;
		}
		return { minima , h };
	}
	inline static matrix simple_gradient_meth(
		std::function<double(const matrix&)> func,
		matrix begin,
		bool attempt_global_minimization,
		std::function<bool(double, double)> norma_comparator = [](double eps, double norma)->bool {return norma < eps; },
		double epsilon = 1e-10,
		access_method_data* amd = nullptr
	) {
		static std::deque<std::pair<matrix, double>> buffer;
		buffer.clear();
		matrix prev;
		double func_value;

		buffer.clear();

		if (amd) {
			amd->locker.lock();
			amd->size_callback = []()->int { return buffer.size(); };
			amd->get_value_callback = [](int i)->double {return buffer[i].second; };
			amd->get_param_callback = [](int i)->matrix& {return buffer[i].first; };
			amd->delete_callback = [](int i) { buffer.erase(buffer.begin() + i); };
			amd->is_alive = true;
			amd->locker.unlock();
		}

		do {
			auto grad = (gradient(func, begin)) * (-1);
			//std::cout << "Orig step: " << grad.transpose() << std::endl;
			auto [is_not_degenerated, optD, top, bottom] = align_gradient(grad, begin, epsilon);
			if (!is_not_degenerated)
				return begin;
			auto D = optD.value();

			//std::cout << "Final step: " << D.transpose() << std::endl;

			auto odfunc = [&](double p) -> double {
				return func_value = func(begin + p * D);
			};

			double a, b;
			if (attempt_global_minimization) {
				auto [minima, h] = onedim_grid_minima(odfunc, bottom, top, 20, epsilon);
				a = minima - h;
				b = minima + h;
			}
			else {
				a = bottom;
				b = top;
			}
			auto [arg_minima, func_minima] = onedim_minimistaion(odfunc, a, b, epsilon);
			prev = (begin);
			auto next_step = D * arg_minima;
			begin += next_step;

			if (amd) {
				amd->locker.lock();
				size_t maximum = params_manipulator_globals::begin_evolution_sizes;
				if (buffer.size() == maximum) {
					size_t max_idx = 0;
					for (size_t i = 0; i < maximum; i++) {
						if (buffer[max_idx].second < buffer[i].second)
							max_idx = i;
					}
					if (buffer[max_idx].second < func_minima) {
						size_t rnd1 = mpiss::erand() * buffer.size();
						size_t rnd2 = mpiss::erand() * buffer.size();
						double a = mpiss::erand();
						auto new_point = buffer[rnd1].first * a + (1 - a) * buffer[rnd2].first;
						next_step = new_point - begin;
						begin = new_point;
					}
					else {
						buffer.erase(buffer.begin() + max_idx);
						buffer.push_back({ begin, func_minima });
					}
				}
				else
					buffer.push_back({ begin, func_minima });
				amd->locker.unlock();
			}

			if (norma_comparator(epsilon, next_step.norma(2)))
				break;
			
			std::cout << "Step norma: " << next_step.norma(1) << std::endl;
			//if ((prev - begin).norma(1) < epsilon)
				//break;
			//std::cout << "Next approx: " << begin.transpose();
		} while (true);
		return begin;
	}

	inline static matrix newton_method(
		std::function<double(const matrix&)> func,
		matrix begin,
		std::function<bool(double, double)> norma_comparator = [](double eps, double norma)->bool {return norma < eps; },
		double epsilon = 1e-10,
		access_method_data* amd = nullptr
	) {
		static std::deque<std::pair<matrix, double>> buffer;
		buffer.clear();
		matrix prev;
		double func_value;

		buffer.clear();

		if (amd) {
			amd->locker.lock();
			amd->size_callback = []()->int { return buffer.size(); };
			amd->get_value_callback = [](int i)->double { return buffer[i].second; };
			amd->get_param_callback = [](int i)->matrix& { return buffer[i].first; };
			amd->delete_callback = [](int i) { buffer.erase(buffer.begin() + i); };
			amd->is_alive = true;
			amd->locker.unlock();
		}

		do {
			auto [f, D, H] = func_gradient_and_hessian(func, begin);

			prev = begin;
			if (H.norma() < 0.0001)
				H = matrix::E_matrix(H.cols());

			auto inverseH = H.inverse();
			auto direction = -1. * inverseH * D;
			auto [is_not_degenerated, new_direction, _1, _2] = align_gradient(direction, begin, epsilon);
			if (!is_not_degenerated)
				break;

			auto next_step = new_direction.value();
			begin = begin + next_step;

			if (amd) {
				amd->locker.lock();
				size_t maximum = params_manipulator_globals::begin_evolution_sizes;
				if (buffer.size() == maximum) {
					size_t max_idx = 0;
					for (size_t i = 0; i < maximum; i++) {
						if (buffer[max_idx].second < buffer[i].second)
							max_idx = i;
					}
					if (buffer[max_idx].second < f) {
						size_t rnd1 = mpiss::erand() * buffer.size();
						size_t rnd2 = mpiss::erand() * buffer.size();
						double a = mpiss::erand();
						auto new_point = buffer[rnd1].first * a + (1 - a) * buffer[rnd2].first;
						next_step = new_point - begin;
						begin = new_point;
					}
					else {
						buffer.erase(buffer.begin() + max_idx);
						buffer.push_back({ begin, f });
					}
				}
				else
					buffer.push_back({ begin, f });
				amd->locker.unlock();
			}

			std::cout << f << std::endl;

			if (norma_comparator(epsilon, (begin - prev).norma()))
				break;
		} while (true);
		return begin;
	}

	inline static matrix differential_evolution(
		std::function<double(const matrix&)> func,
		matrix sample,
		double initial_wiggle_coef,
		double replace_index_probability,
		size_t entries_amount,
		double epsilon = 1e-10,
		access_method_data* amd = nullptr
	) {
		static std::vector<matrix> entries;
		static std::vector<double> func_values;
		entries.clear();
		func_values.clear();
		entries.resize(entries_amount, sample);
		func_values.resize(entries_amount, 1e127);

		if (amd) {
			amd->locker.lock();
			amd->size_callback = []()->int { return entries.size(); };
			amd->get_value_callback = [](int i)->double {return func_values[i]; };
			amd->get_param_callback = [](int i)->matrix& {return entries[i]; };
			amd->delete_callback = [](int i) { entries.erase(entries.begin() + i); func_values.erase(func_values.begin() + i); };
			amd->is_alive = true;
			amd->locker.unlock();
		}

		auto mutate = [&](size_t mx_id) -> matrix {
			double coef = mpiss::erand() * 2;
			size_t a_id, b_id, c_id;
			do {
				a_id = (size_t)(mpiss::erand() * entries_amount);
			} while (mx_id == a_id);
			do {
				b_id = (size_t)(mpiss::erand() * entries_amount);
			} while (mx_id == b_id || a_id == b_id);
			do {
				c_id = (size_t)(mpiss::erand() * entries_amount);
			} while (mx_id == c_id || a_id == c_id || c_id == b_id);

			matrix crossovered = entries[mx_id];
			crossovered.selfapply_indexed([&](double& val, size_t row, size_t col) {
				if (mpiss::erand() > replace_index_probability)
					return;
				val = std::clamp(entries[a_id].at(col, row) + coef * (entries[b_id].at(col, row) - entries[c_id].at(col, row)), 0., 1.);
				});
			return crossovered;
		};

		for (auto& mx : entries) {
			mx.selfapply([&](double& v) {v = std::clamp(mpiss::nrand() * initial_wiggle_coef, 0., 1.); });
		}

		double min, max;
		size_t min_id, max_id;

		uint32_t cnt = 0;
		do {
			min = 1e127, max = -1e127;
			min_id = 0, max_id = 0;
			for (size_t i = 0; i < amd->size_callback(); i++) {
				auto new_approx = mutate(i);
				auto new_value = func(new_approx);
				if (new_value < func_values[i]) {
					if (amd)
						amd->locker.lock();
					entries[i].swap(new_approx);
					func_values[i] = new_value;
					if (amd)
						amd->locker.unlock();
				}
				else
					new_value = func_values[i];
				if (new_value < min) {
					min_id = i;
					min = new_value;
				}
				if (new_value > max) {
					max_id = i;
					max = new_value;
				}
			}
			if((cnt)%16 == 0)
				printf("Range: %lf\n", max - min);
			cnt++;
			//printf("New minima: %lf\n", min);
			//std::cout << std::endl << entries[min_id] << std::endl;
		} while (std::abs(max - min) > epsilon);
		std::cout << std::endl << entries[min_id] << std::endl;
		return entries[min_id];
	}

	inline static matrix extended_annealing(
		std::function<double(const matrix&)> func,
		matrix sample,
		double initial_sample_wiggling,
		double wiggle_decay_coef,
		double start_temperature,
		size_t entries_amount,
		double epsilon = 1e-10,
		access_method_data* amd = nullptr
	) {
		double cur_wiggle_coef = initial_sample_wiggling;
		static std::vector<matrix> entries;
		static std::vector<double> func_values;
		entries.clear();
		func_values.clear();
		entries.resize(entries_amount, sample);
		func_values.resize(entries_amount, INFINITY);

		if (amd) {
			amd->locker.lock();
			amd->size_callback = []()->int { return entries.size(); };
			amd->get_value_callback = [](int i)->double { return func_values[i]; };
			amd->get_param_callback = [](int i)->matrix& { return entries[i]; };
			amd->delete_callback = [](int i) { entries.erase(entries.begin() + i); func_values.erase(func_values.begin() + i); };
			amd->is_alive = true;
			amd->locker.unlock();
		}

		matrix wiggle_mx = sample;

		while (cur_wiggle_coef > epsilon) {

			for (size_t i = 0; i < amd->size_callback(); i++) {
				wiggle_mx.selfapply([&](double& v) {
					v = mpiss::erand() - 0.5;
					});
				wiggle_mx.normalize();
				auto new_approx = entries[i] + wiggle_mx * cur_wiggle_coef;
				new_approx.selfapply([](double& v) {
					v = std::clamp(v, 0., 1.);
					});
				double func_val = func(new_approx);
				bool random_jump = false;
				double prob = 0;
				bool is_smaller = func_val < func_values[i];
				if (!is_smaller) 
					prob = std::exp(-(func_val - func_values[i]) / (cur_wiggle_coef / start_temperature));
				if (is_smaller || mpiss::erand() < prob) { // pure evolution algorithm (couldn't make annealing work)
					amd->locker.lock();
					func_values[i] = func_val;
					new_approx.swap(entries[i]);
					amd->locker.unlock();
				}
				//std::cout << func_val << " " << func_values[i] << " " << cur_wiggle_coef * start_temperature << " " << prob << " " << random_jump << std::endl;
			}
			
			cur_wiggle_coef *= wiggle_decay_coef;
			printf("Wiggle coeficient: %f\n", (float)cur_wiggle_coef);
		}
		auto min = std::min_element(func_values.begin(), func_values.end());

		return entries[min - func_values.begin()];
	}
};

#endif