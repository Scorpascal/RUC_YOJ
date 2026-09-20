// ...existing code...
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

struct Person {
    string id;
    string name;
    char gender;
    int age;
    int cnt;
    int idx;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Person> v;
    for (int i = 0; i < 3; ++i) {
        Person p;
        if (!(cin >> p.id >> p.name >> p.gender >> p.age >> p.cnt)) return 0;
        p.idx = i;
        v.push_back(p);
    }

    sort(v.begin(), v.end(), [](const Person& a, const Person& b) {
        if (a.cnt != b.cnt) return a.cnt > b.cnt;
        return a.idx < b.idx;
    });

    for (const auto& p : v) {
        cout << p.id << ',' << p.name << ',' << p.gender << ',' << p.age << ',' << p.cnt << '\n';
    }
    return 0;
}
// ...existing code...