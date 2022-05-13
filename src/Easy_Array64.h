#pragma once
#include <initializer_list>
#include <cstdint>

template <typename T>
class EasyArray64 {
private:
	void free() {
		if (data == nullptr) {
			return;
		}
		length = 0;
		delete[](data);
		data = nullptr;
	}

public:
	void copy_from(const T* a) {
		for (uint64_t i = 0; i < length; ++i) {
			data[i] = a[i];
		}
	}

	void clear_copy_from(uint64_t l, const T* a) {
		free();
		if (l == 0) return;
		data = new T[l];
		length = l;
		copy_from(a);
	}

	void swap(EasyArray64<T>& a) {
		T* temp_ptr = data;
		uint64_t temp_leng = length;
		data = a.data;
		length = a.length;
		a.data = temp_ptr;
		a.length = temp_leng;
	}

	EasyArray64(std::initializer_list<T> l) {
		length = (uint64_t)l.size();
		if (length == 0) {
			data = nullptr;
			return;
		}
		data = new T[length];
		uint64_t i = 0;

		auto it = l.begin();
		for (uint64_t i = 0; i < length && it != l.end(); ++i) {
			data[i] = *it;
			++it;
		}
	}

	EasyArray64() {
		length = 0;
		data = nullptr;
	}
	EasyArray64(uint64_t l) {
		length = l;
		data = l > 0 ? new T[l] : nullptr;
	};
	~EasyArray64() {
		free();
	}

	void call_on_all(void (*f)(T*)) {
		for (uint64_t i = 0; i < length; ++i) {
			f(&data[i]);
		}
	}

	void new_length(uint64_t l, const T& filler_value) {
		if (l == 0) {
			free();
			return;
		}
		if (l == length) {
			return;
		}
		T* temp = new T[l];
		uint64_t i = 0;
		for (; i < l && i < length; ++i) {
			temp[i] = data[i];
		}
		for (; i < l; ++i) {
			temp[i] = filler_value;
		}
		free();
		data = temp;
		length = l;
	}

	void new_min_length(uint64_t l, const T& filler_value) {
		if (l == 0) {
			free();
			return;
		}
		if (l <= length) {
			return;
		}
		T* temp = new T[l];
		uint64_t i = 0;
		for (; i < l && i < length; ++i) {
			temp[i] = data[i];
		}
		for (; i < l; ++i) {
			temp[i] = filler_value;
		}
		free();
		data = temp;
		length = l;
	}

	void operator=(const EasyArray64<T>& a) {
		new_length(a.length, {});
		if (a.length == 0) {
			return;
		}
		copy_from(a.data);
	}

	void set_all(const T& a) {
		for (uint64_t i = 0; i < length; ++i) {
			data[i] = a;
		}
	}

	bool contains(const T& a, uint64_t& out_index) const{
		for (uint64_t i = 0; i < length; ++i) {
			if (data[i] == a) {
				out_index = i;
				return true;
			}
		}
		return false;
	}

	T* data;
	uint64_t length;
};