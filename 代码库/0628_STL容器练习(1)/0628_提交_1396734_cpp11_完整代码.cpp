#include <iostream>
#include <string>

// add the header file below
#include <vector>

using namespace std;

// put you GetTo and Print implementation below
template<typename T>
void GetTo(vector<T>& container) {
    T value;
    cin >> value;
    container.push_back(value);
}

template<typename T>
void Print(const vector<T>& container) {
    for (size_t i = 0; i < container.size(); ++i) {
        if (i) cout << " ";
        cout << container[i];
    }
    cout << endl;
}

int main() {
	int n;
	string tag;
	cin >> n >> tag;
	if ("int" == tag) {
		// define the container with a proper container type
		vector<int> container;
		for (int i = 0; i < n; i++) {
			GetTo(container);
		}
		Print(container);
	} else if ("double" == tag) {
		// define the container with a proper container type
		vector<double> container;
		for (int i = 0; i < n; i++) {
			GetTo(container);
		}
		Print(container);
	}
	return 0;
}

