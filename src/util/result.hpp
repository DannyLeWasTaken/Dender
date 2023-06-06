//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_RESULT_HPP
#define DENDER_RESULT_HPP

#include <functional>

template <typename T, typename E>
struct Result {
	T Ok;
	E Error;

	T expect(
			const std::function<void(bool)>& error_handler
			) {
		if (Error) {
			error_handler(this->Error);
		} else {
			return this->Ok;
		}
	}
};

#endif//DENDER_RESULT_HPP
