//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_ID_GEN_HPP
#define DENDER_ID_GEN_HPP

#include <cstdint>

class id_gen {

	/// @brief Get next ID in id generator
	/// @return id
	uint64_t next() {
		this->count++;
		return count;
	}

	/// @brief Resets ID gen
	void reset() {
		this->count = 0;
	}

private:
	uint64_t count = -1;
};

#endif//DENDER_ID_GEN_HPP
