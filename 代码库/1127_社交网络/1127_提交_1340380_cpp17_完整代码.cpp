#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
using namespace std;

// Batagelj & Zaversnik (2003) k-core decomposition, O(n+m)

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    int m;
    cin >> n >> m;

    vector<vector<int>> adj(n + 1);
    adj.reserve(n + 1);

    vector<int> deg(n + 1, 0);

    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;
        adj[u].push_back(v);
        adj[v].push_back(u);
        deg[u]++;
        deg[v]++;
    }

    int maxDeg = 0;
    for (int v = 1; v <= n; v++) maxDeg = max(maxDeg, deg[v]);

    // bin[d] = start index (1-based) for vertices of degree d in the "vert" array
    vector<int> bin(maxDeg + 1, 0);
    for (int v = 1; v <= n; v++) bin[deg[v]]++;

    int start = 1;
    for (int d = 0; d <= maxDeg; d++) {
        int num = bin[d];
        bin[d] = start;
        start += num;
    }

    vector<int> pos(n + 1, 0);
    vector<int> vert(n + 1, 0);

    // Place vertices into vert[] by degree (counting sort)
    for (int v = 1; v <= n; v++) {
        int d = deg[v];
        int p = bin[d];
        pos[v] = p;
        vert[p] = v;
        bin[d]++;
    }

    // Restore bin[] to point to the first position of each degree block
    for (int d = maxDeg; d >= 1; d--) bin[d] = bin[d - 1];
    bin[0] = 1;

    vector<int> core(n + 1, 0);

    for (int i = 1; i <= n; i++) {
        int v = vert[i];
        core[v] = deg[v];

        for (int u : adj[v]) {
            if (deg[u] > deg[v]) {
                int du = deg[u];
                int pu = pos[u];
                int pw = bin[du];
                int w = vert[pw];

                if (u != w) {
                    // swap u and w in vert[]
                    vert[pu] = w;
                    vert[pw] = u;
                    pos[u] = pw;
                    pos[w] = pu;
                }

                bin[du]++;
                deg[u]--;
            }
        }
    }

    for (int v = 1; v <= n; v++) {
        cout << core[v] << '\n';
    }
    return 0;
}