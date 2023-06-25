//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_RESULT_HPP
#define DENDER_RESULT_HPP

#include <variant>
#include <optional>
#include <stdexcept>

template <typename Ok, typename Err>
struct Result {
public:
	Result(Ok ok_value) : result(ok_value) {}
	Result(Err err_value) : result(err_value) {}

	std::optional<Ok> unwrap() {
		if (std::holds_alternative<Ok>(result)) {
			return std::get<Ok>(result);
		} else {
			throw std::runtime_error("Unwrapped called for an error result");
		}
	}

	std::optional<Ok> expect(const std::string& error_message) {
		if (std::holds_alternative<Ok>(result)) {
			return std::get<Ok>(result);
		} else {
			throw std::runtime_error(error_message);
		}
	}

private:
	std::variant<Ok, Err> result;
};

#endif//DENDER_RESULT_HPP
