//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_RESULT_HPP
#define DENDER_RESULT_HPP

#include <variant>
#include <stdexcept>

template <typename T, typename E = bool>
class Result {
public:
	Result(std::in_place_index_t<0>, T ok) : result(std::in_place_index<0>, ok) {}
	Result(std::in_place_index_t<1>, E err) : result(std::in_place_index<1>, err) {}

	static Result Ok(T okValue) {
		return Result(std::in_place_index<0>, okValue);
	}

	static Result Err(E errValue) {
		return Result(std::in_place_index<1>, errValue);
	}

	T unwrap() {
		if (std::holds_alternative<T>(result)) {
			return std::get<T>(result);
		} else {
			throw std::runtime_error("Called unwrap on a Result object in Err state.");
		}
	}

	T expect(const std::string& error_message) {
		if (std::holds_alternative<T>(result)) {
			return std::get<T>(result);
		} else {
			throw std::runtime_error(error_message);
		}
	}

	bool is_err() {
		return std::holds_alternative<E>(result);
	}

private:
	std::variant<T, E> result;
};

#endif//DENDER_RESULT_HPP
