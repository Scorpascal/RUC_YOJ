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

    while (n--) {
        string s;
        cin >> s;

        bool hasLetter = false, hasDigit = false, hasSpecial = false;
        int cnt[128] = {0};
        int maxCnt = 0;

        for (unsigned char ch : s) {
            if (isalpha(ch)) hasLetter = true;
            else if (isdigit(ch)) hasDigit = true;
            else if (ch == '*' || ch == '#') hasSpecial = true;

            if (ch < 128) {
                maxCnt = max(maxCnt, ++cnt[ch]);
            }
        }

        if (!(hasLetter && hasDigit && hasSpecial)) {
            cout << 0 << "\n";
        } else if (maxCnt <= 2) {
            cout << 2 << "\n";
        } else {
            cout << 1 << "\n";
        }
    }
    return 0;
}