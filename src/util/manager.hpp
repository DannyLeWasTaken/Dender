//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_MANAGER_HPP
#define DENDER_MANAGER_HPP

#include "handle.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include "result.hpp"
#include "id_gen.hpp"

///@brief Manager for handlers
template <typename T>
class Manager {

	/// @brief Hand over object to the manager to manage
	/// @param obj Object to hand over via std::move()
	/// @returns Handle of the handed over object
	Handle<T> add(T obj);

	/// @brief Add object through a shared ptr to be handled by manager
	Handle<T> add(std::shared_ptr<T> obj);


	Result<Handle<T>, bool> get(Handle<T> handle);

	/// @brief Removes the handle and object it's linked to from the manager
	/// @param handle Handle and it's corresponding object to remove
	void remove(Handle<T>& handle);

	/// @brief Removes the handle and its corresponding object from the handle, but as well destroys
	/// @brief the object itself
	/// @param handle Handle and corresponding object to destroy
	void destroy(Handle<T>& handle);

private:
    uint64_t counter;
    std::vector<std::shared_ptr<T>> storage;
    std::vector<Handle<T>> handles;

	/// @brief Validates the given handle to make sure it exists in the manager
	/// @param handle Handle to validate
	bool is_valid_handle(Handle<T>& handle) {
		return handle.get_index() < this->storage.size() &&
			   handle.is_valid() &&
			   handle.get_count() == this->handles[handle.get_index()].get_count();
	}
};


#endif //DENDER_MANAGER_HPP
