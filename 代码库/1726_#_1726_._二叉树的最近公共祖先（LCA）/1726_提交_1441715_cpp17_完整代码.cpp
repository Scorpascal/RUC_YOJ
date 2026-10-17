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
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
using namespace std;

int getLCA(int u, int v, const vector<vector<int> >& up, const vector<int>& depth, int logn) {
    if (depth[u] < depth[v]) swap(u, v);
    int diff = depth[u] - depth[v];
    for (int k = 0; diff; ++k, diff >>= 1) {
        if (diff & 1) u = up[k][u];
    }
    if (u == v) return u;
    for (int k = logn - 1; k >= 0; --k) {
        if (up[k][u] != up[k][v]) {
            u = up[k][u];
            v = up[k][v];
        }
    }
    return up[0][u];
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    int n, q;
    if (!(cin >> n >> q)) return 0;
    vector<int> leftChild(n + 1), rightChild(n + 1), depth(n + 1, 0);
    for (int i = 1; i <= n; ++i) cin >> leftChild[i] >> rightChild[i];

    int logn = 1;
    while ((1LL << logn) <= n) ++logn;
    vector<vector<int> > up(logn, vector<int>(n + 1, 0));
    stack<int> st;
    st.push(1);
    while (!st.empty()) {
        int u = st.top();
        st.pop();
        int children[2] = {leftChild[u], rightChild[u]};
        for (int i = 0; i < 2; ++i) {
            int v = children[i];
            if (v) {
                up[0][v] = u;
                depth[v] = depth[u] + 1;
                st.push(v);
            }
        }
    }
    for (int k = 1; k < logn; ++k) {
        for (int v = 1; v <= n; ++v) {
            up[k][v] = up[k - 1][up[k - 1][v]];
        }
    }
    while (q--) {
        int u, v;
        cin >> u >> v;
        cout << getLCA(u, v, up, depth, logn);
        cout.put(10);
    }
    return 0;
}