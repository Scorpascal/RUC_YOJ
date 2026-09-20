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

    int n;
    cin >> n;

    vector<int> normal(n);  // 四舍五入
    vector<int> banker(n);  // 银行家舍入

    for (int i = 0; i < n; ++i) {
        string s;
        cin >> s;

        size_t pos = s.find('.');

        int a = stoi(s.substr(0, pos));
        int b = s[pos + 1] - '0';

        // 四舍五入
        if (b >= 5)
            normal[i] = a + 1;
        else
            normal[i] = a;

        // 银行家舍入
        if (b <= 4) {
            banker[i] = a;
        } else if (b >= 6) {
            banker[i] = a + 1;
        } else { // b == 5
            if (a % 2 == 0)
                banker[i] = a;
            else
                banker[i] = a + 1;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (i) cout << ' ';
        cout << normal[i];
    }
    cout << '\n';

    for (int i = 0; i < n; ++i) {
        if (i) cout << ' ';
        cout << banker[i];
    }
    cout << '\n';

    return 0;
}