#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    long long ans = 0;
    for (int i = 0; i < n - 1; ++i) {
        int u, v;
        int w;
        cin >> u >> v >> w;
        ans += (long long)w;
    }

    cout << ans << '\n';
    return 0;
}