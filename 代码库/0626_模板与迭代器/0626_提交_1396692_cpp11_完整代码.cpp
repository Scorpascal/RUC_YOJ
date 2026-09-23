#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::cin;
using std::string;

namespace ai {
// put you iterator below
template<typename T>
class iterator {
    T* p;
public:
    explicit iterator(T* ptr = nullptr) : p(ptr) {}
    T& operator*() const { return *p; }
    iterator& operator++() { ++p; return *this; }
    iterator operator++(int) { iterator old(*this); ++p; return old; }
    bool operator==(const iterator& other) const { return p == other.p; }
    bool operator!=(const iterator& other) const { return p != other.p; }
};
}

template<typename T, typename Iterator = ai::iterator<T>>
Iterator find(Iterator begin, Iterator end, const T val) {
	for (Iterator iter = begin; iter != end; iter++) {
		if (*iter == val)
			return iter;
	}
	return end;
}

// put your Get below
template<typename T>
T* Get(int n) {
    T* array = new T[n];
    for (int i = 0; i < n; ++i) cin >> array[i];
    return array;
}

template<typename T>
void UseIterator(T* array, int size) {
	ai::iterator<T> begin(array);
	ai::iterator<T> end(array+size);
	T val;
	cin >> val;
	for (int i = 0; i < size; i++) {
		cout << array[i];
		if (i < size - 1)
			cout << " ";
	}
	cout << endl;
	ai::iterator<T> iter = find(begin, end, val);
	if (iter != end) {
		cout << "Found " << *iter << endl;
	} else {
		cout << "Not Found " << val << endl;
	}
}

int main() {
	int m, n;
	string tag;
	cin >> m >> n;

	for (int i = 0; i < m; i++) {
		cin >> tag;
		if ("int" == tag) {
			UseIterator(Get<int>(n), n);
		} else if ("double" == tag) {
			UseIterator(Get<double>(n), n);
		}
	}
	
	return 0;
}

