#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, L;
    if (!(cin >> n >> L)) return 0;
    vector<vector<int>> a(n, vector<int>(n));
    for (auto &row : a) for (int &x : row) cin >> x;

    const vector<string> pat = {
        "*********",
        "*--*--*-*",
        "*--*****-",
        "*----**--",
        "*******--"
    };
    vector<int> diff(L + 1, 0);
    for (int r = 0; r + 4 < n; ++r) {
        for (int c = 0; c + 8 < n; ++c) {
            int lo = 0, hi = L - 1;
            for (int i = 0; i < 5; ++i) {
                for (int j = 0; j < 9; ++j) {
                    int v = a[r + i][c + j];
                    if (pat[i][j] == '*') hi = min(hi, v);
                    else lo = max(lo, v + 1);
                }
            }
            if (lo <= hi) {
                ++diff[lo];
                --diff[hi + 1];
            }
        }
    }
    int active = 0;
    for (int k = 0; k < L; ++k) {
        active += diff[k];
        if (active > 0) cout << k << '\n';
    }
    return 0;
}