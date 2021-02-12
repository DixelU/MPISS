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
	float plotter_xpos, plotter_ypos;
	float plotter_width, plotter_height;
	float mouse_x, mouse_y;
	float mouse_cx, mouse_cy; // converted
	bool is_fixed_aspect_ratio, is_hovered;
	float points_size;
	int grid_rel_depth;
	DWORD points_color, lines_color, bright_lines_color;
	STLptr left_top_indicator, left_bottom_indicator, bottom_left_indicator, bottom_right_indicator, indicator_top, indicator_right;
	std::vector<std::pair<float, float>> points;
	float internal_width, internal_height;
	float internal_center_x, internal_center_y;
	float indicator_x, indicator_y;
	~DottedPlotter() override {
		amd->is_alive = false;
	}
	DottedPlotter(float xpos, float ypos, float width, float height, float points_size, SingleTextLineSettings* STLS, DWORD points_color, DWORD lines_color, DWORD bright_lines_color, bool is_fixed_aspect_ratio = true) :
		left_top_indicator(STLS->CreateOne()), left_bottom_indicator(STLS->CreateOne()),
		bottom_left_indicator(STLS->CreateOne()), bottom_right_indicator(STLS->CreateOne()),
		indicator_top(STLS->CreateOne()), indicator_right(STLS->CreateOne()),
		plotter_xpos(xpos), plotter_ypos(ypos), plotter_width(width), plotter_height(height),
		points_color(points_color), lines_color(lines_color), bright_lines_color(bright_lines_color),
		points_size(points_size), internal_width(1), internal_height(1), internal_center_x(1), internal_center_y(1),
		mouse_x(0), mouse_y(0), is_hovered(false), is_fixed_aspect_ratio(is_fixed_aspect_ratio), grid_rel_depth(3)
	{
		amd = new access_method_data;
	}
	void DumpPoints() {
		lock_guard locker(Lock);
		if (!amd->is_alive)
			return;
		points.clear();
		int size = amd->size_callback();
		for (int i = 0; i < size; i++) {
			float param = amd->get_param_callback(i).at(mx_x, mx_y);
			float value = amd->get_value_callback(i);
			points.push_back({ param , value });
		}
		std::sort(points.begin(), points.end());

		UpdateEdges();
	}

	std::pair<float, float> convert_point_to_canvas_coordinates(float x, float y) const {
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

	std::pair<float, float> convert_point_to_internal_coordinates(float x, float y) const {
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

		left_top_indicator->SafeStringReplace(std::to_string(internal_center_y + internal_height * 0.5));
		bottom_right_indicator->SafeStringReplace(std::to_string(internal_center_x + internal_width * 0.5));

		left_bottom_indicator->SafeStringReplace(std::to_string(internal_center_y - internal_height * 0.5));
		bottom_left_indicator->SafeStringReplace(std::to_string(internal_center_x - internal_width * 0.5));
		//todo: fix following lines
		left_top_indicator->SafeChangePosition_Argumented(GLOBAL_TOP & GLOBAL_RIGHT,
			plotter_xpos - plotter_width * 0.5, plotter_ypos + plotter_height * 0.5);
		left_bottom_indicator->SafeChangePosition_Argumented(GLOBAL_BOTTOM & GLOBAL_RIGHT,
			plotter_xpos - plotter_width * 0.5, plotter_ypos - plotter_height * 0.5);
		bottom_right_indicator->SafeChangePosition_Argumented(GLOBAL_TOP & GLOBAL_RIGHT,
			plotter_xpos + plotter_width * 0.5, plotter_ypos - plotter_height * 0.5);
		bottom_left_indicator->SafeChangePosition_Argumented(GLOBAL_TOP & GLOBAL_LEFT,
			plotter_xpos - plotter_width * 0.5, plotter_ypos - plotter_height * 0.5);

		if (is_hovered) {
			indicator_top->SafeStringReplace(std::to_string(mouse_cx));
			indicator_right->SafeStringReplace(std::to_string(mouse_cy));

			indicator_top->SafeChangePosition_Argumented(GLOBAL_BOTTOM,
				plotter_xpos + plotter_height * 0.5, mouse_y);
			indicator_right->SafeChangePosition_Argumented(GLOBAL_LEFT,
				mouse_x, plotter_ypos +  plotter_width * 0.5);
		}
	}

	void Draw() override {
		lock_guard locker(Lock);
		DumpPoints();
		RealignIndicators();

		glBegin(GL_POINTS);
		GLCOLOR(points_color);
		glPointSize(points_size);
		for (auto& [x, y] : points) {
			auto [conv_x, conv_y] = convert_point_to_canvas_coordinates(x, y);
			glVertex2f(conv_x, conv_y);
		}
		glEnd();

		glBegin(GL_LINE_LOOP);
		GLCOLOR(bright_lines_color | (is_hovered * 0xFF));
		glLineWidth(1);
		glVertex2f(plotter_xpos + 0.5 * plotter_width, plotter_ypos + 0.5 * plotter_height);
		glVertex2f(plotter_xpos - 0.5 * plotter_width, plotter_ypos + 0.5 * plotter_height);
		glVertex2f(plotter_xpos - 0.5 * plotter_width, plotter_ypos - 0.5 * plotter_height);
		glVertex2f(plotter_xpos + 0.5 * plotter_width, plotter_ypos - 0.5 * plotter_height);
		glEnd();

		left_top_indicator->Draw();
		left_bottom_indicator->Draw();
		bottom_right_indicator->Draw();
		bottom_left_indicator->Draw();

		if (is_hovered) {
			glBegin(GL_LINES);
			GLCOLOR(bright_lines_color);
			glVertex2f(plotter_xpos + 0.5 * plotter_width, mouse_y);
			glVertex2f(plotter_xpos - 0.5 * plotter_width, mouse_y);
			glVertex2f(mouse_x, plotter_ypos - 0.5 * plotter_height);
			glVertex2f(mouse_x, plotter_ypos + 0.5 * plotter_height);
			glEnd();

			indicator_top->Draw();
			indicator_right->Draw();
		}
	}
	
	void UpdateEdges() {
		lock_guard locker(Lock);
		if (points.empty())
			return;
		float left_x = points.front().first;
		float right_x = points.back().first;
		auto [min_y_it, max_y_it] = std::minmax_element(points.begin(), points.end(), 
			[](const auto& a, const auto& b) { return a.second < b.second; });
		float min_y = max_y_it->second;
		float max_y = max_y_it->second;

		float new_width = right_x - left_x;
		float new_height = max_y - min_y;

		if (is_fixed_aspect_ratio) {
			float ratio = plotter_height / plotter_width;
			if (new_width > new_height)
				new_height = new_width * ratio;
			else
				new_width = new_height / ratio;
		}

		internal_width = (new_width + internal_width) * 0.5; //slow convergence;
		internal_height = (new_height + internal_height) * 0.5;
		float avg_x = 0, avg_y = 0;
		for (auto& [x, y] : points) {
			avg_x += x;
			avg_y += y;
		}
		avg_x /= points.size();
		avg_y /= points.size();
		internal_center_x = (internal_center_x + avg_x) * 0.5;
		internal_center_y = (internal_center_y + avg_y) * 0.5;
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
		float CW = 0.5f * (
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
		if (fabsf(mx - plotter_xpos) < 0.5 * plotter_width && fabsf(my - plotter_ypos) < 0.5 * plotter_width) {
			is_hovered = true;
			mouse_x = mx;
			mouse_y = my;
			auto [mcx, mcy] = convert_point_to_internal_coordinates(mouse_x, mouse_y);
			mouse_cx = mcx;
			mouse_cy = mcy;
		}
		else {
			is_hovered = false;
		}
		return 0;
	}
	bool IsResizeable() override {
		return true;
	}
	void virtual SafeResize(float NewHeight, float NewWidth) {
		return;
	}
};


#endif