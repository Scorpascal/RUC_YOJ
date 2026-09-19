#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    long long b, c, l, r;
    cin >> b >> c >> l >> r;

    long long start = l;
    if (start % 2 != 0) start++; // 取区间内第一个偶数横坐标点

    long long sum = 0;
    for (long long x = start; x <= r; x += 2) {
        sum += x * x + b * x + c;
    }

    cout << 2 * sum << "\n";
    return 0;
}