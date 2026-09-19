#include <bits/stdc++.h>
using namespace std;

using int64 = long long;

int64 n;
int k, m;

bool check(int64 x) {
    int64 t = n;

    for (int day = 1; day <= m; ++day) {
        // 当天先变质 ceil(t * k / 100) 个
        int64 bad = (t * k + 99) / 100;
        t -= bad;

        // 剩余苹果不够所有机器人各吃一个
        if (t < x)
            return false;

        t -= x;
    }

    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n >> k >> m;

    int64 l = 0, r = 1000000000LL;

    while (l < r) {
        int64 mid = l + (r - l + 1) / 2;

        if (check(mid))
            l = mid;
        else
            r = mid - 1;
    }

    cout << l << '\n';

    return 0;
}