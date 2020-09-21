#pragma once
#ifndef MPISS_CEMETERY_NDEF
#define MPISS_CEMETERY_NDEF

#include "mpiss_header.h"
#include "mpiss_cell.h"

namespace mpiss {
	struct cemetery {
		std::vector<mpiss::cell*> deads;
		cemetery() {}
		void take_from(std::vector<mpiss::cell*>& cells, size_t id) {
			size_t last_id = cells.size() - 1;
			std::swap(cells[id], cells[last_id]);
			deads.push_back(cells.back());
			cells.pop_back();
		}
	};
}

#endif