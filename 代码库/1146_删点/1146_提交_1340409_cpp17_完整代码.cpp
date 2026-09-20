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

// ...existing code...

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int L, N, M;
    cin >> L >> N >> M;

    vector<int> x(N + 2);
    x[0] = 0;
    for (int i = 1; i <= N; i++) cin >> x[i];
    x[N + 1] = L;

    auto can = [&](int d) -> bool {
        int removed = 0;
        int last = x[0];
        for (int i = 1; i <= N + 1; i++) {
            if (x[i] - last < d) {
                removed++;
                if (removed > M) return false;
            } else {
                last = x[i];
            }
        }
        return removed <= M;
    };

    int lo = 0, hi = L;
    while (lo < hi) {
        int mid = lo + (hi - lo + 1) / 2;
        if (can(mid)) lo = mid;
        else hi = mid - 1;
    }

    cout << lo << "\n";
    return 0;
}