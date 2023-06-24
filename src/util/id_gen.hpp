//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_ID_GEN_HPP
#define DENDER_ID_GEN_HPP

#include <atomic>
#include <cstdint>

template<typename T, typename IdType = uint32_t>
struct id_gen {
public:
	/// @brief Get next ID in id generator
	/// @return id
	static IdType next() {
		return cur++;
	}

private:
	static inline std::atomic<IdType> cur = 0;
};

#endif//DENDER_ID_GEN_HPP
