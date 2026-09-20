#include <iostream>
#include <vector>
using namespace std;

int main() {
    int n, m;
    cin >> n >> m;

    vector<int> v(n);

    for (auto it = v.begin(); it != v.end(); ++it) {
        cin >> *it;
    }

    if (m == 1) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            (*it)--;
        }
    }
    else if (m == 2) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            (*it) *= 2;
        }
    }
    else {
        long long sum = 0;

        for (auto it = v.begin(); it != v.end(); ++it) {
            sum += *it;
        }

        cout << sum << endl;
        return 0;
    }

    for (auto it = v.begin(); it != v.end(); ++it) {
        if (it != v.begin()) cout << ' ';
        cout << *it;
    }

    return 0;
}