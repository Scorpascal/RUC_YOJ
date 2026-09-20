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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, k;
    if (!(cin >> n >> k)) return 0;
    if (n < 0 || k < 0) { cout << 0 << '\n'; return 0; }
    vector<vector<long long>> p(n + 1, vector<long long>(k + 1, 0));
    p[0][0] = 1;
    for (int s = 1; s <= n; ++s) {
        for (int parts = 1; parts <= k; ++parts) {
            long long a = p[s - 1][parts - 1];
            long long b = (s - parts >= 0) ? p[s - parts][parts] : 0;
            p[s][parts] = a + b;
        }
    }
    cout << p[n][k] << '\n';
    return 0;
}