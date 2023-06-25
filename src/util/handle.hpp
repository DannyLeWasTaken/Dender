//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_HANDLE_HPP
#define DENDER_HANDLE_HPP

#include <cstdint>
#include <functional>
#include "id_gen.hpp"


/**
 * @brief A handle type that acts as an abstraction for an object
 * @tparam T Type of the value that the handle holds
 */
template <typename T>
struct Handle {
public:
	Handle(uint64_t index, uint64_t count, uint64_t id) : index(index), count(count), id(id) {}

	int get_index() const {
		return this->index;
	}

	int get_count() const {
		return this->count;
	}

	int get_id() const {
		return this->id;
	}

	void set_index(const int in) {
		this->index = in;
	}

	void set_count(const int co) {
		this->count = co;
	}

	/// @brief   Determines if given handle has an index and counter over -1
	/// @returns bool
	bool is_valid() {
		return this->index >= 0 && this->count >= 0;
	}

	/**
	 * @brief Copies a handle
	 * @param rhs The handle to copy
	 */
	Handle(Handle const& rhs) = default;

	/**
	 * @brief Default empty constructor
	 */
	 Handle() = default;

	/**
	 * @brief Copies a handle
	 * @param rhs The handle to copy
	 * @return *this
	 */
	Handle& operator=(Handle const& rhs) = default;

	bool operator==(const Handle<T>& other) const {
		return other.index == this->index &&
			   other.count == this->count;
	}

	bool operator!=(const Handle<T>& other) const {
		return other.index != this->index || other.count != this->index;
	}

	explicit operator bool() const {
		return this->is_valid();
	}

private:
	friend class std::hash<Handle<T>>;
	
	uint64_t index = -1; // Default to invalid. Index location
	uint64_t count = -1; // Default to invalid
	uint64_t id = -1; // Default to invalid
};

namespace std {
	// https://github.com/NotAPenguin0/Andromeda/blob/1e17278c6697293885112d9d6a2b7d0504527917/include/andromeda/util/handle.hpp#L11
	// thanks penguin
	// This is a hashing function :D
	template <typename T>
	struct hash<Handle<T>> {
		size_t operator()(Handle<T> const& x) const {
			hash<uint64_t> hash{};
			return hash( x.id );
		}
	};
}

#endif//DENDER_HANDLE_HPP
