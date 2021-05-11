#pragma once
#ifndef MPISS_MDA_H
#define MPISS_MDA_H

#include <atomic>
#include <mutex>
#include <functional>

#include "matrix.h"

struct access_method_data {
	std::atomic_bool is_alive = false;
	std::mutex locker;
	std::function<matrix& (int)> get_param_callback;
	std::function<double(int)> get_value_callback;
	std::function<int()> size_callback;
	std::function<void(int)> delete_callback;
};

#endif 