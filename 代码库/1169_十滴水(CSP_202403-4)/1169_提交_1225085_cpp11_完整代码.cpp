#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    long long c; int m, n;
    if (!(cin >> c >> m >> n)) return 0;

    vector<pair<long long,int>> a(m);
    for (int i = 0; i < m; ++i) cin >> a[i].first >> a[i].second;
    sort(a.begin(), a.end()); // 按位置排序

    // 建立索引与双向链表
    vector<long long> pos(m+1);
    vector<int> val(m+1), L(m+1), R(m+1);
    vector<char> alive(m+1, 1);
    unordered_map<long long,int> id;
    id.reserve(m*2); id.max_load_factor(0.7f);

    for (int i = 1; i <= m; ++i) {
        pos[i] = a[i-1].first;
        val[i] = a[i-1].second; // 初始 1..4
        L[i] = i-1; R[i] = i+1;
        id[pos[i]] = i;
    }
    if (m >= 1) { L[1] = 0; R[m] = 0; }

    int alive_cnt = m;
    set<int> boom; // 维护当前水滴数≥5的格子（按位置最小优先）

    auto explode = [&](int i) {
        if (!alive[i]) return;
        if (val[i] >= 5) boom.erase(i); // 自己将要爆开，先移出集合

        int l = L[i], r = R[i];

        // 清空并从链表删除
        alive[i] = 0;
        --alive_cnt;
        if (l) R[l] = r;
        if (r) L[r] = l;

        // 同时对两侧最近的“仍有水”的格子 +1
        if (l) {
            if (++val[l] >= 5) boom.insert(l);
        }
        if (r) {
            if (++val[r] >= 5) boom.insert(r);
        }
    };

    for (int op = 0; op < n; ++op) {
        long long p; cin >> p;
        int idx = id[p]; // 保证此时该位置有水

        // 本次操作：先对该格子 +1
        ++val[idx];
        boom.clear();
        if (val[idx] >= 5) boom.insert(idx);

        // 全局按“最靠左”的顺序处理所有连锁爆开
        while (!boom.empty()) {
            int u = *boom.begin();
            boom.erase(boom.begin());
            if (!alive[u]) continue;
            if (val[u] < 5) continue;
            explode(u);
        }

        cout << alive_cnt << '\n';
    }
    return 0;
}