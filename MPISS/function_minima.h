#pragma once
#ifndef FUNC_MINIMA_DEF_GUARD
#define FUNC_MINIMA_DEF_GUARD

#include <optional>
#include <functional>
#include "matrix.h"
#include "mpiss_header.h"

struct params_manipulator {
	inline static matrix gradient(std::function<double(const matrix&)> f, const matrix& p) {
		constexpr double step = 0.000001;
		matrix e(p.rows(), p.cols()), D(p.rows(), p.cols());
		auto h = p * step;
		h.selfapply([step](double& v) {if (v < DBL_EPSILON) v = step; });
		for (size_t i = 0; i < e.rows(); i++) {
			e.at(0, i) = h.at(0, i);
			auto pph = p + e, pmh = p - e;
			pph.selfapply([](double& v) { v = std::clamp(v, 0., 1.); });
			pmh.selfapply([](double& v) { v = std::clamp(v, 0., 1.); });
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
		double epsilon = 1e-10
	) {
		matrix prev;
		do {
			auto grad = (gradient(func, begin)) * (-1);
			//std::cout << "Orig step: " << grad.transpose() << std::endl;
			auto [is_not_degenerated, optD, top, bottom] = align_gradient(grad, begin, epsilon);
			if (!is_not_degenerated)
				return begin;
			auto D = optD.value();

			//std::cout << "Final step: " << D.transpose() << std::endl;

			auto odfunc = [&](double p) -> double {
				return func(begin + p * D);
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
			auto local_minima = onedim_minimistaion(odfunc, a, b, epsilon);
			prev = (begin);
			auto next_step = D * local_minima;
			begin += next_step;

			if (norma_comparator(epsilon, next_step.norma(2)))
				break;

			std::cout << "Step norma: " << next_step.norma(1) << std::endl;
			//if ((prev - begin).norma(1) < epsilon)
				//break;
			//std::cout << "Next approx: " << begin.transpose();
		} while (true);
		return begin;
	}
	inline static matrix differential_evolution(
		std::function<double(const matrix&)> func,
		matrix sample,
		double initial_wiggle_coef,
		double replace_index_probability,
		size_t entries_amount,
		double epsilon = 1e-10
	) {
		std::vector<matrix> entries;
		std::vector<double> func_values;
		entries.resize(entries_amount, sample);
		func_values.resize(entries_amount, 1e127);

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
		do {
			min = 1e127, max = -1e127;
			min_id = 0, max_id = 0;
			for (size_t i = 0; i < entries_amount; i++) {
				auto new_approx = mutate(i);
				auto new_value = func(new_approx);
				if (new_value < func_values[i]) {
					entries[i] = new_approx;
					func_values[i] = new_value;
				}
				else
					new_value = func_values[i];
				if (new_value < min) {
					min_id = i;
					min = new_value;
				}
				else if (new_value > max) {
					max_id = i;
					max = new_value;
				}
			}
			printf("Range: %lf\n", max - min);
			printf("New minima: %lf\n", min);
			std::cout << std::endl << entries[min_id] << std::endl;
		} while (max - min > epsilon);
		return entries[min_id];
	}
	inline static matrix extended_annealing(
		std::function<double(const matrix&)> func,
		matrix sample,
		double initial_sample_siggling,
		double wiggle_decay_coef,
		double wiggle_probability,
		double portion_of_nonwiggleable_matixes,
		size_t entries_amount,
		double epsilon = 1e-10
	) {
		double cur_wiggle_coef = initial_sample_siggling;
		std::vector<matrix> entries;
		std::vector<matrix> buffer_entries;
		std::vector<std::pair<double, int>> rate_vec;
		entries.resize(entries_amount, sample);
		rate_vec.resize(entries_amount, { 0.,0 });
		size_t wiggleable_matixes = size_t(entries_amount * portion_of_nonwiggleable_matixes);

		auto mx_wiggle_sapply_func = [&](double& val) {
			if (mpiss::erand() > wiggle_probability)
				return;
			val = std::clamp(val + mpiss::nrand() * cur_wiggle_coef, 0., 1.);
		};

		for (auto& mx : entries) {
			mx.selfapply([&](double& v) {
				if (mpiss::erand() > wiggle_probability)
					return;
				v = std::clamp(mpiss::nrand() * initial_sample_siggling, 0., 1.); }
			);
		}

		while (cur_wiggle_coef > epsilon) {
			for (size_t i = 0; i < entries_amount; i++) {
				double val = func(entries[i]);
				rate_vec[i] = { val, i };
			}
			cur_wiggle_coef *= wiggle_decay_coef;

			std::sort(rate_vec.begin(), rate_vec.end());

			printf("Current minima: %lf\n", rate_vec.front().first);
			printf("Current wiggle coef: %lf\n", cur_wiggle_coef);
			std::cout << std::endl << entries[rate_vec.front().second] << std::endl;

			for (size_t i = wiggleable_matixes; i < entries_amount; i++) {
				auto& [value, id] = rate_vec[i];
				size_t source_id = size_t(mpiss::erand() * wiggleable_matixes);
				entries[id] = entries[source_id].apply(mx_wiggle_sapply_func);
			}
		}

		for (size_t i = 0; i < entries_amount; i++) {
			double val = func(entries[i]);
			rate_vec[i] = { val, i };
		}

		std::sort(rate_vec.begin(), rate_vec.end());

		return entries[rate_vec.front().second];
	}
};

#endif