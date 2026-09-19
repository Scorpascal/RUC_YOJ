#include <iostream>
#include <vector>
#include <set>
#include <functional>
#include <algorithm>
#include <limits>
#include <climits>
#include <cstdlib>
using namespace std;

using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, m;
    if (!(cin >> n >> m)) return 0;
    vector<ll> w(n + 1);
    for (int i = 1; i <= n; ++i) cin >> w[i];

    vector<int> parent(n + 1, 0);
    vector<vector<int>> ch(n + 1);
    for (int i = 2; i <= n; ++i) {
        int p; cin >> p;
        parent[i] = p;
        ch[p].push_back(i);
    }

    // Euler tour, subtree sum with all nodes
    vector<int> tin(n + 1), tout(n + 1), euler(1);
    vector<ll> subAll(n + 1, 0);
    int timer = 0;
    function<ll(int)> dfs = [&](int u) -> ll {
        tin[u] = ++timer;
        euler.push_back(u);
        ll s = w[u];
        for (int v : ch[u]) s += dfs(v);
        tout[u] = timer;
        subAll[u] = s;
        return s;
    };
    dfs(1);

    auto isAncestor = [&](int a, int b) -> bool {
        return tin[a] <= tin[b] && tout[b] <= tout[a];
    };

    const int ID_MIN = numeric_limits<int>::min();

    auto pickAskNode = [&](const set<pair<ll,int>>& S, const vector<ll>& curSub, ll total) -> int {
        if (S.empty()) return -1;
        ll halfCeil = (total + 1) / 2; // ceil(total/2)
        auto itUp = S.lower_bound({halfCeil, ID_MIN});
        vector<int> cands;
        if (itUp != S.end()) cands.push_back(itUp->second);
        if (itUp == S.begin()) {
            if (itUp == S.end()) {
                auto itLast = prev(S.end());
                ll v = itLast->first;
                auto itFirstInBucket = S.lower_bound({v, ID_MIN});
                cands.push_back(itFirstInBucket->second);
            }
        } else {
            auto itPrev = prev(itUp);
            ll v = itPrev->first;
            auto itFirstInBucket = S.lower_bound({v, ID_MIN});
            cands.push_back(itFirstInBucket->second);
        }
        sort(cands.begin(), cands.end());
        cands.erase(unique(cands.begin(), cands.end()), cands.end());

        pair<ll,int> best = {LLONG_MAX, INT_MAX};
        int bestId = -1;
        for (int id : cands) {
            ll ws = llabs(2 * curSub[id] - total);
            pair<ll,int> key = {ws, id};
            if (key < best) { best = key; bestId = id; }
        }
        return bestId;
    };

    // Process queries
    for (int qi = 0; qi < m; ++qi) {
        int t; cin >> t;

        vector<char> alive(n + 1, 1);
        vector<ll> curSub = subAll;
        set<pair<ll,int>> S; // (current subtree sum, id)
        for (int i = 1; i <= n; ++i) S.insert({curSub[i], i});
        ll total = curSub[1];

        vector<int> ans;

        while ((int)S.size() > 1) {
            int s = pickAskNode(S, curSub, total);
            ans.push_back(s);

            bool yes = isAncestor(s, t);

            if (yes) {
                vector<set<pair<ll,int>>::iterator> toDel;
                for (auto it = S.begin(); it != S.end(); ++it) {
                    int id = it->second;
                    if (!isAncestor(s, id)) toDel.push_back(it);
                }
                for (auto it : toDel) {
                    alive[it->second] = 0;
                    S.erase(it);
                }
                total = curSub[s];
            } else {
                ll delta = curSub[s];
                for (int idx = tin[s]; idx <= tout[s]; ++idx) {
                    int u = euler[idx];
                    if (!alive[u]) continue;
                    alive[u] = 0;
                    S.erase({curSub[u], u});
                }
                total -= delta;
                int v = parent[s];
                while (v != 0 && alive[v]) {
                    S.erase({curSub[v], v});
                    curSub[v] -= delta;
                    S.insert({curSub[v], v});
                    v = parent[v];
                }
            }
        }

        for (size_t i = 0; i < ans.size(); ++i) {
            if (i) cout << ' ';
            cout << ans[i];
        }
        cout << "\n";
    }
    return 0;
}