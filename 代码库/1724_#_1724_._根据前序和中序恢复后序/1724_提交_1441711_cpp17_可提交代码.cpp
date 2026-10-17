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
struct Task { int pl, il, len; bool emit; };
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    if (!(cin >> n)) return 0;
    vector<int> pre(n), pos(n + 1);
    for (int &x : pre) cin >> x;
    for (int i = 0, x; i < n; ++i) { cin >> x; pos[x] = i; }
    vector<int> post;
    post.reserve(n);
    vector<Task> st;
    st.push_back({0, 0, n, false});
    while (!st.empty()) {
        Task cur = st.back(); st.pop_back();
        if (cur.len == 0) continue;
        if (cur.emit) { post.push_back(pre[cur.pl]); continue; }
        int root = pre[cur.pl];
        int mid = pos[root];
        int left = mid - cur.il;
        int right = cur.len - left - 1;
        st.push_back({cur.pl, cur.il, cur.len, true});
        if (right) st.push_back({cur.pl + left + 1, mid + 1, right, false});
        if (left) st.push_back({cur.pl + 1, cur.il, left, false});
    }
    for (int i = 0; i < n; ++i) {
        if (i) cout << ' ';
        cout << post[i];
    }
    cout << endl;
    return 0;
}