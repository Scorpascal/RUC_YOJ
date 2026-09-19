#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    vector<int> result;
    result.reserve(n);
    unordered_set<int> seen;
    for (int i = 0; i < n; ++i) {
        int x;
        cin >> x;
        if (seen.insert(x).second) result.push_back(x);
    }
    cout << result.size() << endl;
    for (size_t i = 0; i < result.size(); ++i) {
        if (i) cout << ' ';
        cout << result[i];
    }
    cout << endl;
    return 0;
}