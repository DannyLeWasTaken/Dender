//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_RESULT_HPP
#define DENDER_RESULT_HPP

#include <variant>
#include <optional>
#include <stdexcept>
#include <memory>

/**
 * @brief A handle struct which vaguely mimics the one found in rust.\n
 * E defaults to a boolean and true for an error for safety reason.
 * @tparam T Success type
 * @tparam E Error type. Default: bool
 */
template <typename T, typename E = bool>
struct Result {
public:
	Result() : result(static_cast<E>(true)) {}

	Result(T ok) : result(ok) {}
	Result(E err) : result(err) {}


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
