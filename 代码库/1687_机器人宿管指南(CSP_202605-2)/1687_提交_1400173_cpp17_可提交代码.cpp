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

using int64 = long long;

int64 n;
int k, m;

bool check(int64 x) {
    int64 t = n;

    for (int day = 1; day <= m; ++day) {
        // 当天先变质 ceil(t * k / 100) 个
        int64 bad = (t * k + 99) / 100;
        t -= bad;

        // 剩余苹果不够所有机器人各吃一个
        if (t < x)
            return false;

        t -= x;
    }

    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n >> k >> m;

    int64 l = 0, r = 1000000000LL;

    while (l < r) {
        int64 mid = l + (r - l + 1) / 2;

        if (check(mid))
            l = mid;
        else
            r = mid - 1;
    }

    cout << l << '\n';

    return 0;
}