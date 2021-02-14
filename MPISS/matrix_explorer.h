#pragma once
#ifndef MATRIX_EXPLORER_H
#define MATRIX_EXPLORER_H

#include <functional>
#include <optional>
#include <map>

#include "SAFGUIF/handleable_ui_part.h"
#include "SAFGUIF/checkbox.h"

struct MatrixExplorer: HandleableUIPart {
	float leftmost_x;
	float topmost_y;
	float side_sizes;
	std::pair<size_t, size_t> cur_checkbox, dims;
	std::function<std::pair<size_t, size_t>()> dims_getter;
	std::function<void(void)> resize_callback;
	std::function<void(size_t, size_t)> click_callback;
	std::map<std::pair<size_t, size_t>, CheckBox*> checkboxes;
	int border_size;
	DWORD unchecked_color, checked_color, border_color;

	MatrixExplorer(float x, float y, float checkboxes_side_sizes, int border_size, DWORD unchecked_color, DWORD checked_color, DWORD border_color, std::function<std::pair<size_t, size_t>()> dims_getter, std::function<void(size_t, size_t)> click_callback = [](size_t x, size_t y) {}, std::function<void(void)> resize_callback = []() {}) :
		leftmost_x(x), topmost_y(y), cur_checkbox({0,0}), dims_getter(dims_getter), side_sizes(checkboxes_side_sizes), resize_callback(resize_callback),
		border_size(border_size), unchecked_color(unchecked_color), checked_color(checked_color), border_color(border_color), click_callback(click_callback)
	{
		FetchDims();
	}

	void FetchDims() {
		lock_guard locker(Lock);
		auto new_dims = dims_getter();
		if (new_dims.first == 0 || new_dims.second == 0)
			new_dims = { 1,1 };
		if (new_dims != dims) {
			auto [old_x, old_y] = dims;
			auto [new_x, new_y] = new_dims;
			for (auto [indexes, cb] : checkboxes)
				delete cb;
			checkboxes.clear();
			for (size_t x = 0; x < new_x; x++) {
				for (size_t y = 0; y < new_y; y++) {
					checkboxes[{x, y}] = new CheckBox(
						leftmost_x + (x + 0.5) * side_sizes, 
						topmost_y - (y + 0.5) * side_sizes,
						side_sizes, border_color, unchecked_color, checked_color, border_size, false);
				}
			}
			dims = new_dims;

			resize_callback();
		}
		auto cur_checkbox_ptr = checkboxes[cur_checkbox];
		if (!cur_checkbox_ptr->State)
			cur_checkbox_ptr->State = true;
	}

	void Draw() override {
		lock_guard locker(Lock);
		FetchDims();
		for (auto [indexes, cb] : checkboxes)
			cb->Draw();
	}

	void SafeMove(float dx, float dy) override {
		lock_guard locker(Lock);
		leftmost_x += dx;
		topmost_y += dy;
		for (auto [indexes, cb] : checkboxes)
			cb->SafeMove(dx, dy);
	}
	void SafeChangePosition(float NewX, float NewY) override {
		lock_guard locker(Lock);
		NewX -= leftmost_x;
		NewY -= topmost_y;
		SafeMove(NewX, NewY);
	}
	void SafeChangePosition_Argumented(BYTE Arg, float NewX, float NewY) override {
		lock_guard locker(Lock);
		float CW = 0.5f * (
			(INT32)((BIT)(GLOBAL_LEFT & Arg))
			- (INT32)((BIT)(GLOBAL_RIGHT & Arg))
			) * (dims.first) * this->side_sizes, 
			CH = 0.5f * (
				(INT32)((BIT)(GLOBAL_BOTTOM & Arg))
				- (INT32)((BIT)(GLOBAL_TOP & Arg))
				) * (dims.second) * this->side_sizes;
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
		float cur_width = side_sizes * dims.first;
		float cur_height = side_sizes * dims.second;
		float rel_x = mx - leftmost_x;
		float rel_y = topmost_y - my;
		if (rel_x < cur_width && rel_x > 0 && rel_y > 0 && rel_y < cur_height) {
			if (Button) {
				auto [old_x, old_y] = cur_checkbox;
				int x_idx = (rel_x / side_sizes);
				int y_idx = (rel_y / side_sizes);

				checkboxes[cur_checkbox]->State = false;
				cur_checkbox = { x_idx, y_idx };

				click_callback(x_idx, y_idx);
			}
		}

		BIT flag = false;
		for (auto [indexes, cb] : checkboxes)
			flag |= cb->MouseHandler(mx,my, Button, State);

		return flag;
	}
	bool IsResizeable() override {
		return false;
	}
};

#endif