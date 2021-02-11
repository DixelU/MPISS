#pragma once
#ifndef MPISS_DOTTED_PLOTTER
#define MPISS_DOTTED_PLOTTER

#include <vector>

#include "SAFGUIF/handleable_ui_part.h"
#include "matrix.h"
#include "access_method_data.h"

struct DottedPlotter : HandleableUIPart {
	access_method_data *amd;
	int mx_x, mx_y;
	float plotter_xpos, plotter_ypos;
	float plotter_width, plotter_height;
	bool bottom_is_fixed_zero;
	float constraint_graph_left, constraint_graph_right;
	float points_size;
	DWORD points_color;
	std::vector<std::pair<float, float>> points;
	~DottedPlotter() override {
		amd->is_alive = false;
	}
	DottedPlotter() {
		amd = new access_method_data;
	}
	void DumpPoints() {
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
	}

	void Draw() override {
		DumpPoints();
		glBegin(GL_POINTS);
		GLCOLOR(points_color);
		glPointSize(points_size);
		for (auto& [x, y] : points) {
			glVertex2f(x, y);
		}
		glEnd();
	}
	void SafeMove(float dx, float dy) override {
		plotter_xpos += dx;
		plotter_ypos += dy;
	}
	void SafeChangePosition(float NewX, float NewY) override {
		plotter_xpos = NewX;
		plotter_ypos = NewY;
	}
	void SafeChangePosition_Argumented(BYTE Arg, float NewX, float NewY) override {

	}
	void KeyboardHandler(CHAR CH) override {
		return;
	}
	void SafeStringReplace(std::string Meaningless) override {
		return;
	}
	BIT MouseHandler(float mx, float my, CHAR Button, CHAR State) override {
		if (fabsf(mx - plotter_xpos) < 0.5 * plotter_width && fabsf(my - plotter_ypos) < 0.5 * plotter_width) {


		}
		else {

		}
		return 0;
	}

};


#endif