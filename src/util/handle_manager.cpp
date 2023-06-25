//
// Created by Danny on 2023-04-30.
//

#include "handle_manager.hpp"


template<typename T>
Handle<T> HandleManager<T>::add(T obj) {
	// Add object to be managed
	this->storage.push_back(std::make_shared<T>(std::move(obj)));
	this->counter++;

	// Create new handle
	Handle<T> new_handle {
			.count = this->counter,
			.index = this->storage.size() - 1,
			.id = this->id_generator.next()
	};
	return new_handle;
}

template<typename T>
Handle<T> HandleManager<T>::add(std::shared_ptr<T> obj) {
	// Add object to be managed
	this->storage.push_back(obj);
	this->counter++;

	// Create a new handle
	Handle<T> new_handle {
			.count = this->counter,
			.index = this->storage.size() - 1,
			.id = this->id_generator.next()
	};
	return new_handle;
}

template<typename T>
Result<Handle<T>, bool> HandleManager<T>::get(const Handle<T>& handle) {
	if (this->is_valid_handle(handle)) {
		return {
				.Ok = storage[handle.get_index()],
				.Error = false
		};
	} else {
		return {
				.Ok = Handle<T>(),
				.Error = true
		};
	}
}

template<typename T>
void HandleManager<T>::remove(Handle<T>& handle) {
	if (this->is_valid_handle(handle)) {
		// Remove object of handle
		this->storage[handle.get_index()] = nullptr;

		// Handle exists, invalidate it
		this->handles[handle.get_index()].set_count(-1);
		this->handles[handle.get_index()].set_index(-1);

		handle.set_count(-1);
		handle.set_index(-1);
	}
}

template<typename T>
void HandleManager<T>::destroy(Handle<T>& handle) {
	if (this->is_valid_handle(handle)) {
		// Remove object of handle
		this->storage[handle.get_index()].reset();
		this->storage[handle.get_index()] = nullptr;

		this->handles[handle.get_index()].set_count(-1);
		this->handles[handle.get_index()].set_index(-1);

		handle.set_count(-1);
		handle.set_index(-1);
	}
}