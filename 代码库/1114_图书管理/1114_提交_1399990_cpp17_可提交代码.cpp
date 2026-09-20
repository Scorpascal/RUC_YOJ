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
    if (!(cin >> n)) return 0;

    string line;
    getline(cin, line); // 吃掉行尾换行

    unordered_set<string> books;
    books.reserve(static_cast<size_t>(n) * 2);

    for (int i = 0; i < n; ++i) {
        getline(cin, line);
        if (line.empty()) { // 若有空行，忽略并补读
            --i;
            continue;
        }

        size_t pos = line.find(' ');
        string cmd = (pos == string::npos) ? line : line.substr(0, pos);
        string name = (pos == string::npos) ? "" : line.substr(pos + 1);

        if (cmd == "add") {
            books.insert(name);
        } else if (cmd == "find") {
            cout << (books.find(name) != books.end() ? "yes" : "no") << '\n';
        }
    }

    return 0;
}