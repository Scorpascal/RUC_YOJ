#include <bits/stdc++.h>
using namespace std;

static long long countWaysForStart(const vector<int>& a, int N, int x1, int x2) {
    // Check boundary equation for i=1: a1 = x1 + x2
    if (a[1] != x1 + x2) return 0;

    vector<int> x(N + 2, 0);
    x[1] = x1;
    if (N >= 2) x[2] = x2;

    // Build using: a[i] = x[i-1] + x[i] + x[i+1]  (2 <= i <= N-1)
    for (int i = 2; i <= N - 1; i++) {
        int next = a[i] - x[i - 1] - x[i];
        if (next < 0 || next > 1) return 0;
        x[i + 1] = next;
    }

    // Check last boundary equation for i=N: aN = x[N-1] + x[N]
    if (N == 1) {
        // handled outside
        return 0;
    } else if (N == 2) {
        // a2 must equal x1 + x2
        return (a[2] == x[1] + x[2]) ? 1LL : 0LL;
    } else {
        return (a[N] == x[N - 1] + x[N]) ? 1LL : 0LL;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    if (!(cin >> N)) return 0;

    vector<int> a(N + 1, 0);
    for (int i = 1; i <= N; i++) cin >> a[i];

    // N=1: a1 = x1 (only left neighbor exists)
    if (N == 1) {
        cout << ((a[1] == 0 || a[1] == 1) ? 1 : 0) << "\n";
        return 0;
    }

    long long ans = 0;

    // Enumerate feasible (x1, x2) in {0,1}^2; only those matching a1 can work.
    for (int x1 = 0; x1 <= 1; x1++) {
        for (int x2 = 0; x2 <= 1; x2++) {
            ans += countWaysForStart(a, N, x1, x2);
        }
    }

    cout << ans << "\n";
    return 0;
}