//
// Created by Danny on 2023-04-30.
//

#ifndef DENDER_HANDLE_MANAGER_HPP
#define DENDER_HANDLE_MANAGER_HPP

#include "handle.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include "result.hpp"
#include "id_gen.hpp"

///@brief Manager for handlers
template <typename T>
class HandleManager {
public:
	/**
	 * @brief When HandleManager is destroyed, all handles by default are
	 * removed using remove() and not destroy()
	 */
	~HandleManager() {
		for (auto& handle: this->handles) {
			this->remove(handle);
		}
	};

	/// @brief Hand over object to the manager to manage
	/// @param obj Object to hand over via std::move()
	/// @returns Handle of the handed over object
	Handle<T> add(T obj) {
		// Add object to be managed
		auto ptr = std::make_shared<T>(std::move(obj));
		return this->add(std::move(ptr));
	};

	/// @brief Add object through a shared ptr to be handled by manager
	Handle<T> add(std::shared_ptr<T> obj) {
		// Add object to be managed
		this->storage.push_back(std::move(obj));
		this->counter++;

		// Create a new handle
		Handle<T> new_handle = Handle<T>(
				this->storage.size() - 1,
				this->counter,
				this->id_generator.next()
		);
		this->handles.push_back(new_handle);
		return new_handle;
	};

	/**
	 * @brief Get the original type of from the handle
	 * @param handle
	 * @return
	 */
	Result<std::shared_ptr<T>> get(const Handle<T>& handle) {
		if (this->is_valid_handle(handle)) {
			return Result<std::shared_ptr<T>>::Ok(this->storage[handle.get_index()]);
		} else {
			return Result<std::shared_ptr<T>>::Err(true);
		}
	};

	Result<std::shared_ptr<T>> get(const uint64_t index) {
		if (index >= this->handles.size()) {
			return Result<std::shared_ptr<T>>::Err(true);
		}
		Handle<T> handle = this->handles[index];
		return this->get(handle);
	};

	/**
	 * @brief Gets all the handles that the handle_manager holds
	 * @return const std::vector<Handle<T>> handles
	 */
	const std::vector<Handle<T>> get_handles() {
		return this->handles;
	};

	/**
	 * @brief Get all valid handles
	 * @return vector
	 */
	std::vector<Handle<T>> get_valid_handles() const {
		std::vector<Handle<T>> filtered;
		std::copy_if(this->handles.begin(), this->handles.end(), std::back_inserter(filtered),
					 [this](const Handle<T>& obj) {
						 return this->is_valid_handle(obj);
					 });
	}

	/// @brief Removes the handle and object it's linked to from the manager
	/// @param handle Handle and it's corresponding object to remove
	Result<std::shared_ptr<T>> remove(Handle<T>& handle) {
		if (this->is_valid_handle(handle)) {
			// Get the object of handle
			std::shared_ptr<T> obj = this->storage[handle.get_index()];

			// Remove object of handle
			this->storage[handle.get_index()] = nullptr;

			// Handle exists, invalidate it
			this->handles[handle.get_index()].set_count(-1);
			this->handles[handle.get_index()].set_index(-1);

			handle.set_count(-1);
			handle.set_index(-1);

			return Result<std::shared_ptr<T>>::Ok(obj);
		}
		return Result<std::shared_ptr<T>>::Err(true);
	};

	/// @brief Removes the handle and its corresponding object from the handle, but as well destroys
	/// @brief the object itself
	/// @param handle Handle and corresponding object to destroy
	void destroy(Handle<T>& handle) {
		auto ptr = this->remove(handle);
		ptr.unwrap().reset(); // Destroy object
	};

	/**
	 * @brief Destroys all handles in HandleManager using destroy()\n
	 * Use this if you're planning on destroying all memory associated with handle
	 * manager
	 */
	void destroy_all() {
		for (auto& handle: this->handles) {
			this->destroy(handle);
		}
	};

private:
	uint64_t counter;
	std::vector<std::shared_ptr<T>> storage;
	std::vector<Handle<T>> handles;
	IdGen<T, uint64_t> id_generator;

	/// @brief Validates the given handle to make sure it exists in the manager
	/// @param handle Handle to validate
	bool is_valid_handle(Handle<T> handle) const {
		return handle.get_index() < this->storage.size() &&
			   handle.is_valid() &&
			   handle.get_count() == this->handles[handle.get_index()].get_count();
	}
};


#endif//DENDER_HANDLE_MANAGER_HPP
