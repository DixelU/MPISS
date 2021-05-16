#pragma once
#ifndef MPISS_DOTTED_PLOTTER
#define MPISS_DOTTED_PLOTTER

#include <vector>

#include "SAFGUIF/handleable_ui_part.h"
#include "matrix.h"
#include "access_method_data.h"
#include "SAFGUIF/single_text_line.h"
#include "SAFGUIF/stls_presets.h"

struct DottedPlotter : HandleableUIPart {
	using STLptr = SingleTextLine*;
	access_method_data *amd;
	int mx_x, mx_y;
	double plotter_xpos, plotter_ypos;
	double plotter_width, plotter_height;
	double mouse_x, mouse_y;
	double mouse_cx, mouse_cy; // converted
	bool is_fixed_aspect_ratio, is_hovered;
	int points_size;
	double outter_margin;
	int grid_rel_depth;
	int closest_to_pointer_param;
	DWORD points_color, lines_color, bright_lines_color;
	STLptr left_top_indicator, left_bottom_indicator, bottom_left_indicator, bottom_right_indicator, indicator_top, indicator_right;
	std::vector<std::pair<double, double>> points;
	std::function<void(const matrix&)> on_click;
	double internal_width, internal_height;
	double internal_center_x, internal_center_y;
	double indicator_x, indicator_y;
	double closest_param_x, closest_param_y;
	double max_value;
	~DottedPlotter() override {
		amd->is_alive = false;
	}
	DottedPlotter(double xpos, double ypos, double width, double height, int points_size, double outter_margin, SingleTextLineSettings* STLS, DWORD points_color, DWORD lines_color, DWORD bright_lines_color, bool is_fixed_aspect_ratio = true, std::function<void(const matrix&)> OnClick = [](const matrix& mx) {}) :
		left_top_indicator(STLS->CreateOne()), left_bottom_indicator(STLS->CreateOne()),
		bottom_left_indicator(STLS->CreateOne()), bottom_right_indicator(STLS->CreateOne()),
		indicator_top(STLS->CreateOne()), indicator_right(STLS->CreateOne()),
		plotter_xpos(xpos), plotter_ypos(ypos), plotter_width(width), plotter_height(height),
		points_color(points_color), lines_color(lines_color), bright_lines_color(bright_lines_color),
		points_size(points_size), internal_width(1), internal_height(1), internal_center_x(1), internal_center_y(1),
		mouse_x(0), mouse_y(0), is_hovered(false), is_fixed_aspect_ratio(is_fixed_aspect_ratio), grid_rel_depth(8),
		outter_margin(outter_margin), mx_x(0), mx_y(0), closest_to_pointer_param(-1), closest_param_x(NAN), closest_param_y(NAN)
	{
		amd = new access_method_data;
	}
	void DumpPoints() {
		lock_guard locker(Lock);
		if (!amd->is_alive)
			return;
		points.clear();

		amd->locker.lock();
		int size = amd->size_callback();
		for (int i = 0; i < size; i++) {
			double param = amd->get_param_callback(i).at(mx_x, mx_y);
			double value = amd->get_value_callback(i);
			points.push_back({ param , value });
		}
		amd->locker.unlock();

		closest_to_pointer_param = -1;
		double dist = INFINITY;
		for (int i = 0; i < size; i++) {
			auto [px, py] = convert_point_to_canvas_coordinates(points[i].first, points[i].second);
			double loc_dist = std::pow(mouse_x - px, 2) + std::pow(mouse_y - py, 2);
			if (loc_dist < dist) {
				dist = loc_dist;
				closest_to_pointer_param = i;
			}
		}
		if (closest_to_pointer_param > -1) {
			closest_param_x = points[closest_to_pointer_param].first;
			closest_param_y = points[closest_to_pointer_param].second;
		}
		else {
			closest_param_x = internal_center_x;
			closest_param_y = internal_center_y;
		}

		std::sort(points.begin(), points.end());

		UpdateEdges();
	}

	std::pair<double, double> convert_point_to_canvas_coordinates(double x, double y) const {
		x -= internal_center_x;
		y -= internal_center_y;

		x /= (internal_width * 0.5);
		y /= (internal_height * 0.5);

		x *= (plotter_width * 0.5);
		y *= (plotter_height * 0.5);

		x += plotter_xpos;
		y += plotter_ypos;
		return { x,y };
	}

	std::pair<double, double> convert_point_to_internal_coordinates(double x, double y) const {
		x -= plotter_xpos;
		y -= plotter_ypos;

		x /= (plotter_width * 0.5);
		y /= (plotter_height * 0.5);

		x *= (internal_width * 0.5);
		y *= (internal_height * 0.5);

		x += internal_center_x;
		y += internal_center_y;
		return { x,y };
	}

	void RealignIndicators() {
		lock_guard locker(Lock);

		auto [mcx, mcy] = convert_point_to_internal_coordinates(mouse_x, mouse_y);
		mouse_cx = mcx;
		mouse_cy = mcy;

		auto fp_to_string = [](double f) -> std::string {
			std::stringstream ss;
			ss << f;
			return ss.str();
		};

		left_top_indicator->SafeStringReplace(fp_to_string(internal_center_y + internal_height * 0.5));
		bottom_right_indicator->SafeStringReplace(fp_to_string(internal_center_x + internal_width * 0.5));

		left_bottom_indicator->SafeStringReplace(fp_to_string(internal_center_y - internal_height * 0.5));
		bottom_left_indicator->SafeStringReplace(fp_to_string(internal_center_x - internal_width * 0.5));

		indicator_top->SafeStringReplace(fp_to_string(mouse_cx));
		indicator_right->SafeStringReplace(fp_to_string(mouse_cy));

		left_top_indicator->SafeChangePosition_Argumented(GLOBAL_TOP | GLOBAL_RIGHT,
			plotter_xpos - (plotter_width * 0.5 + outter_margin), plotter_ypos + plotter_height * 0.5);
		left_bottom_indicator->SafeChangePosition_Argumented(GLOBAL_BOTTOM | GLOBAL_RIGHT,
			plotter_xpos - (plotter_width * 0.5 + outter_margin), plotter_ypos - plotter_height * 0.5);
		bottom_right_indicator->SafeChangePosition_Argumented(GLOBAL_TOP | GLOBAL_RIGHT,
			plotter_xpos + plotter_width * 0.5, plotter_ypos - (plotter_height * 0.5 + outter_margin));
		bottom_left_indicator->SafeChangePosition_Argumented(GLOBAL_TOP | GLOBAL_LEFT,
			plotter_xpos - plotter_width * 0.5, plotter_ypos - (plotter_height * 0.5 + outter_margin));

		indicator_top->SafeChangePosition_Argumented(GLOBAL_BOTTOM,
			mouse_x, plotter_ypos + (plotter_height * 0.5 + outter_margin));
		indicator_right->SafeChangePosition_Argumented(GLOBAL_LEFT,
			plotter_xpos + (plotter_width * 0.5 + outter_margin), mouse_y);
	}

	inline static double nearest_power_of(double x, double v) {
		return std::pow(v, std::ceil(std::log(x) / std::log(v)));
	}

	void Draw() override {
		lock_guard locker(Lock);
		DumpPoints();
		RealignIndicators();

		glLineWidth(1);
		GLCOLOR(bright_lines_color | (is_hovered * 0xFF));
		glBegin(GL_LINE_LOOP);
		glVertex2f(plotter_xpos + 0.5 * plotter_width, plotter_ypos + 0.5 * plotter_height);
		glVertex2f(plotter_xpos - 0.5 * plotter_width, plotter_ypos + 0.5 * plotter_height);
		glVertex2f(plotter_xpos - 0.5 * plotter_width, plotter_ypos - 0.5 * plotter_height);
		glVertex2f(plotter_xpos + 0.5 * plotter_width, plotter_ypos - 0.5 * plotter_height);
		glEnd();

		left_top_indicator->Draw();
		left_bottom_indicator->Draw();
		bottom_right_indicator->Draw();
		bottom_left_indicator->Draw();

		int rel_depth_sq = (grid_rel_depth * grid_rel_depth);
		
		double rounded_width = nearest_power_of(internal_width, 2.) / grid_rel_depth;
		double rounded_height = nearest_power_of(internal_height, 2.) / grid_rel_depth;
		double i_x_lb = std::floor((internal_center_x - 0.5 * internal_width) / rounded_width) * rounded_width;
		double i_y_lb = std::floor((internal_center_y - 0.5 * internal_height) / rounded_height) * rounded_height;

		glBegin(GL_LINES);
		GLCOLOR(((lines_color & 0xFFFFFF00) | ((lines_color & 0xFF) >> 0) ));
		for (int i = 0; i < grid_rel_depth*2; i++) {

			double x1 = i * rounded_width + i_x_lb;
			double y1 = i * rounded_height + i_y_lb;

			auto [x1c, y1c] = convert_point_to_canvas_coordinates(x1, y1);

			if (std::abs(x1c - plotter_xpos) <= plotter_width * 0.5 + 0.01) {
				glVertex2d(x1c, plotter_ypos + 0.5 * plotter_height);
				glVertex2d(x1c, plotter_ypos - 0.5 * plotter_height);
			}
			if (std::abs(y1c - plotter_ypos) <= plotter_height * 0.5 + 0.01) {
				glVertex2d(plotter_xpos + 0.5 * plotter_width, y1c);
				glVertex2d(plotter_xpos - 0.5 * plotter_width, y1c);
			}
		}
		glEnd();


		if (is_hovered) {
			auto [cpx, cpy] = convert_point_to_canvas_coordinates(closest_param_x, closest_param_y);

			glBegin(GL_LINES);
			GLCOLOR(bright_lines_color);
			glVertex2f(plotter_xpos + 0.5 * plotter_width, mouse_y);
			glVertex2f(plotter_xpos - 0.5 * plotter_width, mouse_y);

			glVertex2f(mouse_x, plotter_ypos - 0.5 * plotter_height);
			glVertex2f(mouse_x, plotter_ypos + 0.5 * plotter_height);

			GLCOLOR(points_color);
			glVertex2f(mouse_x, mouse_y);
			glVertex2f(cpx, cpy);
			glEnd();

			indicator_top->Draw();
			indicator_right->Draw();
		}

		glPointSize(points_size);
		GLCOLOR(points_color);
		glBegin(GL_POINTS);
		for (auto& [x, y] : points) {
			auto [conv_x, conv_y] = convert_point_to_canvas_coordinates(x, y);

			if (std::abs(conv_x - plotter_xpos) > 0.5 * plotter_width || std::abs(conv_y - plotter_ypos) > 0.5 * plotter_height)
				continue;

			glVertex2f(conv_x, conv_y);
		}
		glEnd();
	}
	
	void UpdateEdges() {
		lock_guard locker(Lock);

		double new_width = internal_width;
		double new_height = internal_height;

		double left_x = internal_center_x - 0.5 * internal_width;
		double right_x = internal_center_x + 0.5 * internal_width;

		double min_y = internal_center_y - 0.5 * internal_height;
		double max_y = internal_center_y + 0.5 * internal_height;

		if (points.size()) {
			left_x = points.front().first;
			right_x = points.back().first;
			auto [min_y_it, max_y_it] = std::minmax_element(points.begin(), points.end(),
				[](const auto& a, const auto& b) { return a.second < b.second; });
			min_y = min_y_it->second;
			max_y = max_y_it->second;

			new_width = right_x - left_x;
			new_height = max_y - min_y;
		}

		if (std::abs(new_width) >= max_value || std::isnan(new_width))
			new_width = max_value;
		if (std::abs(new_height) >= max_value || std::isnan(new_height))
			new_height = max_value;

		if (std::abs(min_y) >= max_value || std::isnan(min_y))
			min_y = max_value;
		if (std::abs(max_y) >= max_value || std::isnan(max_y))
			max_y = max_value;
		if (std::abs(right_x) >= max_value || std::isnan(right_x))
			right_x = max_value;
		if (std::abs(left_x) >= max_value || std::isnan(left_x))
			left_x = max_value;

		if (is_fixed_aspect_ratio) {
			double plotter_ratio = plotter_width / plotter_height;
			double internal_ratio = new_width / new_height;
			if (plotter_ratio > internal_ratio)
				new_width = new_height * plotter_ratio;
			else
				new_height = new_width / plotter_ratio; 
		}
		
		constexpr double weight_coef = 0.5;
		internal_width = new_width * weight_coef + internal_width * (1 - weight_coef); //slow convergence;
		internal_height = new_height * weight_coef + internal_height * (1 - weight_coef);
		double avg_x = (left_x + right_x) * 0.5, avg_y = (min_y + max_y) * 0.5;
		internal_center_x = internal_center_x * (1 - weight_coef) + avg_x * weight_coef;
		internal_center_y = internal_center_y * (1 - weight_coef) + avg_y * weight_coef;

		/*
		internal_width = new_width; 
		internal_height = new_height;
		double avg_x = (left_x + right_x) * 0.5, avg_y = (min_y + max_y) * 0.5;
		internal_center_x = avg_x;
		internal_center_y = avg_y;
		*/
	}

	void SafeMove(float dx, float dy) override {
		lock_guard locker(Lock);
		plotter_xpos += dx;
		plotter_ypos += dy;
	}
	void SafeChangePosition(float NewX, float NewY) override {
		lock_guard locker(Lock);
		plotter_xpos = NewX;
		plotter_ypos = NewY;
	}
	void SafeChangePosition_Argumented(BYTE Arg, float NewX, float NewY) override {
		lock_guard locker(Lock);
		double CW = 0.5f * (
			(INT32)((BIT)(GLOBAL_LEFT & Arg))
			- (INT32)((BIT)(GLOBAL_RIGHT & Arg))
			) * plotter_width,
		CH = 0.5f * (
			(INT32)((BIT)(GLOBAL_BOTTOM & Arg))
			- (INT32)((BIT)(GLOBAL_TOP & Arg))
			) * plotter_height;
		SafeChangePosition(NewX + CW, NewY + CH);
	}
	void KeyboardHandler(CHAR CH) override {
		return;
	}
	void SafeStringReplace(std::string Meaningless) override {
		return;
	}
	BIT MouseHandler(float mx, float my, CHAR Button, CHAR State) override {
		lock_guard locker(Lock);
		if (fabsf(mx - plotter_xpos) < 0.5 * plotter_width && fabsf(my - plotter_ypos) < 0.5 * plotter_height) {
			is_hovered = true;
			mouse_x = mx;
			mouse_y = my;

			if (Button == -1 && amd && amd->is_alive) {
				amd->locker.lock();
				matrix copy;
				if (closest_to_pointer_param < amd->size_callback()) {
					copy = amd->get_param_callback(closest_to_pointer_param);
				}
				amd->locker.unlock();
				on_click(copy);
				return 1;
			}
			else if (Button == 1 && State<0 && amd && amd->is_alive) {
				amd->locker.lock();
				if (closest_to_pointer_param < amd->size_callback()) {
					amd->delete_callback(closest_to_pointer_param);
				}
				amd->locker.unlock();
				return 1;
			}
		}
		else {
			is_hovered = false;
		}
		return 0;
	}
	bool IsResizeable() override {
		return true;
	}
	void virtual SafeResize(double NewHeight, double NewWidth) {
		plotter_height = NewHeight;
		plotter_width = NewWidth;
		return;
	}
};


#endif