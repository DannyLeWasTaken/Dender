//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_HANDLE_HPP
#define DENDER_HANDLE_HPP

#include <cstdint>
#include <functional>
#include "id_gen.hpp"


/// @brief Handle struct
template <typename T>
struct Handle {
public:
	/**
	 * @brief Constructs a handle as a null handle
	**/
	Handle() = default;

	/**
	 * @brief Copies a handle
	 * @param rhs The handle to copy
	 */
	Handle& operator=(Handle const& rhs) = default;

	/**
	 * @var static Handle none
	 * @brief Respesents a null handle
	 */
	static Handle none;

	/**
	 * @brief Comparator for handles.
	 * @param rhs The handle to compare with
	 * @return trueif the handle refer to the same id
	 */
	bool operator==(const Handle<T>& other) const {
		return other.index == this->index &&
				other.count == this->count;
	}

	/**
	 * @brief Converts a handle to a boolean
	 * @return Return true if the handle is not null, false otherwise.
	 */
	explicit operator bool() const {
		return this->is_valid();
	}
	/**
	 * @brief Get the next available handle of this type.
	 * @return A unique handle of the given type
	**/
	static Handle next() {
		return Handle{id_gen<T, uint64_t>::next()};
	}

	uint64_t get_id() const {return id;}

private:
	friend class std::hash<Handle<T>>;

	uint64_t id = none.id;
	Handle(uint64_t id): id(id) {}
};

template <typename T>
Handle<T> Handle<T>::none = Handle<T>(static_cast<uint64_t>(-1));

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
