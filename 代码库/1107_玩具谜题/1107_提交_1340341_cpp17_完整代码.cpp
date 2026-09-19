#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<int> face(n);          // 0: 朝内, 1: 朝外
    vector<string> job(n);

    for (int i = 0; i < n; i++) {
        cin >> face[i] >> job[i];
    }

    int pos = 0; // 从第一个读入的小人开始（下标0）
    for (int i = 0; i < m; i++) {
        int a, s;
        cin >> a >> s; // a=0 左数, a=1 右数

        int dir;
        if (face[pos] == 0) { // 朝内：左=顺时针(-), 右=逆时针(+)
            dir = (a == 0) ? -1 : +1;
        } else {              // 朝外：左=逆时针(+), 右=顺时针(-)
            dir = (a == 0) ? +1 : -1;
        }

        long long move = 1LL * dir * s;
        pos = (int)((pos + move) % n);
        if (pos < 0) pos += n;
    }

    cout << job[pos] << "\n";
    return 0;
}