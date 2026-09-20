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

struct FastScanner {
    static constexpr size_t BUFSZ = 1 << 20;
    char buf[BUFSZ];
    size_t i = 0, n = 0;

    inline char read() {
        if (i >= n) {
            n = fread(buf, 1, BUFSZ, stdin);
            i = 0;
            if (n == 0) return 0;
        }
        return buf[i++];
    }

    inline bool skipBlanks() {
        char c;
        do {
            c = read();
            if (!c) return false;
        } while (c <= ' ');
        i--; // put back one char
        return true;
    }

    bool readToken(string &out) {
        out.clear();
        if (!skipBlanks()) return false;
        char c;
        while ((c = read()) && c > ' ') out.push_back(c);
        return true;
    }

    template <class T>
    bool readInt(T &out) {
        if (!skipBlanks()) return false;
        char c = read();
        bool neg = false;
        if (c == '-') { neg = true; c = read(); }
        long long x = 0;
        while (c > ' ') {
            x = x * 10 + (c - '0');
            c = read();
        }
        out = neg ? -x : x;
        return true;
    }
};

int main() {
    // 2-bit mapping table (branchless)
    static unsigned char mp[256];
    mp[(unsigned char)'A'] = 0;
    mp[(unsigned char)'C'] = 1;
    mp[(unsigned char)'G'] = 2;
    mp[(unsigned char)'T'] = 3;

    FastScanner fs;

    string s;
    s.reserve(5000005);                 // 避免反复扩容
    if (!fs.readToken(s)) return 0;

    int k;
    fs.readInt(k);

    const int n = (int)s.size();
    if (k <= 0 || k > n) {
        puts("0");
        return 0;
    }

    const uint32_t states = 1u << (2 * k); // 4^k
    const uint32_t mask   = states - 1u;

    vector<int> cnt(states);
    int *c = cnt.data();

    uint32_t code = 0;
    for (int i = 0; i < k; ++i) {
        code = (code << 2) | mp[(unsigned char)s[i]];
    }

    int ans = ++c[code];

    for (int i = k; i < n; ++i) {
        code = ((code << 2) & mask) | mp[(unsigned char)s[i]];
        int v = ++c[code];
        if (v > ans) ans = v;
    }

    printf("%d\n", ans);
    return 0;
}