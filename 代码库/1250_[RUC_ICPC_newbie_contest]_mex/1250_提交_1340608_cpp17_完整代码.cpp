#include <bits/stdc++.h>
using namespace std;

static bool canMakeMexAtLeastK(int k, const vector<int>& a) {
    if (k == 0) return true; // mex >= 0 always

    vector<int> cnt(k, 0);

    for (int x : a) {
        while (x >= k) x >>= 1;     // reduce to the largest value < k on its halving chain
        ++cnt[x];
    }

    // Ensure we can keep one for each i in [1..k-1], push surplus downward by one halving step.
    for (int i = k - 1; i >= 1; --i) {
        if (cnt[i] == 0) return false; // cannot form value i
        --cnt[i];                      // reserve one element to stay as i
        cnt[i >> 1] += cnt[i];         // remaining elements at i can be halved once to floor(i/2)
        // cnt[i] no longer needed afterwards
    }
    return cnt[0] > 0; // need value 0 as well
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T;
    cin >> T;
    while (T--) {
        int n;
        cin >> n;
        vector<int> a(n);
        for (int i = 0; i < n; ++i) cin >> a[i];

        int lo = 0, hi = n + 1; // mex is in [0, n]
        while (lo + 1 < hi) {
            int mid = lo + (hi - lo) / 2;
            if (canMakeMexAtLeastK(mid, a)) lo = mid;
            else hi = mid;
        }
        cout << lo << '\n';
    }
    return 0;
}