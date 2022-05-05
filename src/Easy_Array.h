#pragma once
#include <initializer_list>

template <typename T>
class EasyArray {
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
		for (unsigned int i = 0; i < length; ++i) {
			data[i] = a[i];
		}
	}

	void clear_copy_from(unsigned int l, const T* a) {
		free();
		if (l == 0) return;
		data = new T[l];
		length = l;
		copy_from(a);
	}

	void swap(EasyArray<T>& a) {
		T* temp_ptr = data;
		unsigned int temp_leng = length;
		data = a.data;
		length = a.length;
		a.data = temp_ptr;
		a.length = temp_leng;
	}

	EasyArray(std::initializer_list<T> l) {
		length = (unsigned int)l.size();
		if (length == 0) {
			data = nullptr;
			return;
		}
		data = new T[length];
		unsigned int i = 0;

		auto it = l.begin();
		for (unsigned int i = 0; i < length && it != l.end(); ++i) {
			data[i] = *it;
			++it;
		}
	}

	EasyArray() {
		length = 0;
		data = nullptr;
	}
	EasyArray(unsigned int l) {
		length = l;
		data = l > 0 ? new T[l] : nullptr;
	};
	~EasyArray() {
		free();
	}

	void call_on_all(void (*f)(T*)) {
		for (unsigned int i = 0; i < length; ++i) {
			f(&data[i]);
		}
	}

	void new_length(unsigned int l, const T& filler_value) {
		if (l == 0) {
			free();
			return;
		}
		if (l == length) {
			return;
		}
		T* temp = new T[l];
		unsigned int i = 0;
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

	void new_min_length(unsigned int l, const T& filler_value) {
		if (l == 0) {
			free();
			return;
		}
		if (l <= length) {
			return;
		}
		T* temp = new T[l];
		unsigned int i = 0;
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

	void operator=(const EasyArray<T>& a) {
		new_length(a.length, {});
		if (a.length == 0) {
			return;
		}
		copy_from(a.data);
	}

	void set_all(const T& a) {
		for (unsigned int i = 0; i < length; ++i) {
			data[i] = a;
		}
	}

	bool contains(const T& a, unsigned int& out_index) const{
		for (unsigned int i = 0; i < length; ++i) {
			if (data[i] == a) {
				out_index = i;
				return true;
			}
		}
		return false;
	}

	T* data;
	unsigned int length;
};