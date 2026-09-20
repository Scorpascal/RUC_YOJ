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
    cin >> n >> k;

    while (k--) {
        int x, y;
        string s;
        cin >> x >> y >> s;

        for (char c : s) {
            int nx = x, ny = y;
            if (c == 'f') ny++;
            else if (c == 'b') ny--;
            else if (c == 'l') nx--;
            else if (c == 'r') nx++;

            if (1 <= nx && nx <= n && 1 <= ny && ny <= n) {
                x = nx; y = ny;
            }
        }
        cout << x << ' ' << y << "\n";
    }
    return 0;
}