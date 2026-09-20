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

static inline int lsb_index(uint64_t x) {
    return __builtin_ctzll(x);
}

struct MaxClique {
    int n = 0;
    vector<uint64_t> adj; // adj[i] bit j == 1 means edge i-j exists
    int best = 0;

    // Greedy coloring on induced subgraph of P to get an upper bound.
    // Produces an order of vertices and a color (bound) per vertex.
    void colorSort(uint64_t P, vector<int>& order, vector<int>& colors) {
        order.clear();
        colors.assign(n, 0);

        int color = 0;
        uint64_t uncolored = P;

        while (uncolored) {
            ++color;
            uint64_t candidates = uncolored;

            while (candidates) {
                int v = lsb_index(candidates);
                uint64_t vb = (1ULL << v);

                // assign v this color
                order.push_back(v);
                colors[v] = color;

                // remove v from uncolored and candidates
                uncolored &= ~vb;
                candidates &= ~vb;

                // keep only vertices NOT adjacent to v for the same color class
                candidates &= ~adj[v];
            }
        }
    }

    void expand(int cliqueSize, uint64_t P) {
        if (!P) {
            best = max(best, cliqueSize);
            return;
        }

        vector<int> order;
        vector<int> colors;
        colorSort(P, order, colors);

        // iterate in reverse for stronger pruning
        for (int i = (int)order.size() - 1; i >= 0; --i) {
            int v = order[i];

            // Upper bound pruning
            if (cliqueSize + colors[v] <= best) return;

            // Branch: take v
            expand(cliqueSize + 1, P & adj[v]);

            // Remove v from P
            P &= ~(1ULL << v);

            // Optional: if P is empty, update best
            if (!P) {
                best = max(best, cliqueSize);
                return;
            }
        }
    }

    int solve() {
        best = 0;
        uint64_t all = (n == 64) ? ~0ULL : ((n == 0) ? 0ULL : ((1ULL << n) - 1ULL));
        expand(0, all);
        return best;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    while (cin >> n) {
        if (n == 0) break;

        MaxClique mc;
        mc.n = n;
        mc.adj.assign(n, 0);

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                int x;
                cin >> x;
                if (x == 1) mc.adj[i] |= (1ULL << j);
            }
            // 一般输入保证对角线为0；即便不是，也不影响（取交时会包含自身）
            mc.adj[i] &= ~(1ULL << i); // 明确去掉自环，更稳妥
        }

        cout << mc.solve() << "\n";
    }

    return 0;
}