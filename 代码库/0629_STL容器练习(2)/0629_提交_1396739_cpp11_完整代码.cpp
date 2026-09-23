#include <iostream>
#include <string>

// add the header file below
#include <set>

using namespace std;

// put you GetTo and Print implementation below
template<typename T>
void GetTo(multiset<T>& container) {
    T value;
    cin >> value;
    container.insert(value);
}

template<typename T>
void Print(const multiset<T>& container) {
    bool first = true;
    for (const T& value : container) {
        if (!first) cout << " ";
        first = false;
        cout << value;
    }
    cout << endl;
}

int main() {
	int n;
	string tag;
	cin >> n >> tag;
	if ("int" == tag) {
		// define the container with a proper container type
		multiset<int> container;
		for (int i = 0; i < n; i++) {
			GetTo(container);
		}
		Print(container);
	} else if ("double" == tag) {
		// define the container with a proper container type
		multiset<double> container;
		for (int i = 0; i < n; i++) {
			GetTo(container);
		}
		Print(container);
	}
	return 0;
}

