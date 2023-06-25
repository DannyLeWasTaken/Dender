//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_RESULT_HPP
#define DENDER_RESULT_HPP

#include <variant>
#include <optional>
#include <stdexcept>

template <typename T, typename E = bool>
struct Result {
public:
	Result(T ok_value) : result(ok_value) {}
	Result(E err_value) : result(err_value) {}

	Result(std::enable_if_t<std::is_same<E, bool>::value, E> err_value = true): result(err_value) {}

	std::optional<T> unwrap() {
		if (std::holds_alternative<T>(result)) {
			return std::get<T>(result);
		} else {
			throw std::runtime_error("Unwrapped called for an error result");
		}
	}

	std::optional<T> expect(const std::string& error_message) {
		if (std::holds_alternative<T>(result)) {
			return std::get<T>(result);
		} else {
			throw std::runtime_error(error_message);
		}
	}

private:
	std::variant<T, E> result;
};

#endif//DENDER_RESULT_HPP
