#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    long long m;
    if (!(cin >> n >> m)) return 0;

    vector<long long> a(n), b(n);
    for (int i = 0; i < n; i++) cin >> a[i];
    for (int i = 0; i < n; i++) cin >> b[i];

    long long sumA = 0, sumB = 0, ans = 0;
    int l = 0;

    for (int r = 0; r < n; r++) {
        sumA += a[r];
        sumB += b[r];

        while (l <= r && sumA > m) {
            sumA -= a[l];
            sumB -= b[l];
            l++;
        }

        if (sumA <= m) ans = max(ans, sumB);
    }

    cout << ans << "\n";
    return 0;
}