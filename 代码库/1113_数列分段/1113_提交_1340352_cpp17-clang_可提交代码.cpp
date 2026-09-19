#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    long long M;
    cin >> N >> M;

    long long cur = 0;
    int segments = 0;

    for (int i = 0; i < N; i++) {
        long long a;
        cin >> a;

        // 题面通常保证 a <= M；若不保证，这里可选择输出 -1 或特殊处理
        if (a > M) {
            // 无法满足“每段和不超过 M”
            cout << -1 << "\n";
            return 0;
        }

        if (segments == 0) segments = 1;

        if (cur + a <= M) {
            cur += a;
        } else {
            segments++;
            cur = a;
        }
    }

    cout << segments << "\n";
    return 0;
}