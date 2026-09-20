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