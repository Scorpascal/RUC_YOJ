#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::cin;
using std::string;

/*
 * li::iterator is an iterator for list-type iterator. To simplify the implementation, we 
 * directly treat a li::iterator as a node in the list (see UseIterator). Two li::iterator's
 * are connected by a connect() function in the iterator. Conceptually, a list looks as the 
 * following:
 * 
 *   N1      ->   N2   ->    N3    -> ... ->    Nn
 *  (begin)                                    (end)
 *
 * As shown in UseIterator(), we dereference 'begin' and 'end' to position the corresponding
 * iterator. Here, (li::iterator)*begin is equal to (list::iterator)list.begin() from the STL.
 *
 * Please carefully think about the implementation. 
 */

namespace li {
// put your iterator below
template<typename T>
class iterator {
    struct node {
        T value;
        node* next;
        explicit node(const T& v) : value(v), next(nullptr) {}
    };
    node* current;
public:
    explicit iterator(const T& value) : current(new node(value)) {}
    void connect(iterator& other) { current->next = other.current; }
    T& operator*() const { return current->value; }
    iterator& operator++() { current = current->next; return *this; }
    iterator operator++(int) { iterator old(*this); current = current->next; return old; }
    bool operator==(const iterator& other) const { return current == other.current; }
    bool operator!=(const iterator& other) const { return current != other.current; }
};
}


template<typename T, typename Iterator>
Iterator find(Iterator begin, Iterator end, const T val) {
	for (Iterator iter = begin; iter != end; ++iter) {
		if (*iter == val)
			return iter;
	}
	return end;
}

template<typename T>
void UseIterator(int size) {
	li::iterator<T> *begin = nullptr, *end = nullptr, *last = nullptr;
	for (int i = 0; i < size; i++) {
		T val;
		cin >> val;
		// create a new node (iterator) to hold the value
		li::iterator<T> *iter = new li::iterator<T>(val);
		if (!begin) begin = iter;
		// connect the new node with the last one
		if (last) last->connect(*iter);
		// update the last pointer
		last = iter;
	}
	// we use the last value to initialize 't', so that your have to carefully implement 
	// the iterator to avoid incomplete traversal in below for loops. 
	// In practice (STL), we do not do this.
	T t(**last);
	// 'end' indicates the end of the list. The data stored in 'end' is meaningless.
	end = new li::iterator<T>(t);
	last->connect(*end);
	T val;
	cin >> val;
	// output the list
	int idx = 0;
	for (li::iterator<T> it = *begin; it != *end; ++it) {
		cout << *it;
		if (idx++ < size - 1)
			cout << " ";
	}
	cout << endl;
	// call find 
	li::iterator<T> iter = find(*begin, *end, val);
	if (iter != *end) {
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
			UseIterator<int>(n);
		} else if ("double" == tag) {
			UseIterator<double>(n);
		}
	}
	
	return 0;
}

