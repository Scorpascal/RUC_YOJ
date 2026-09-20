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
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
using namespace std;

struct Fenwick {
    int n;
    vector<int> bit;
    Fenwick(int n) : n(n), bit(n + 1, 0) {}
    void add(int idx, int val) {
        for (; idx <= n; idx += idx & -idx) bit[idx] += val;
    }
    int sum(int idx) const {
        int res = 0;
        for (; idx > 0; idx -= idx & -idx) res += bit[idx];
        return res;
    }
    int range_sum(int l, int r) const {
        if (l > r) return 0;
        return sum(r) - sum(l - 1);
    }
};

struct Event {
    long long x;
    int l, r, id, sign;
    bool operator<(const Event& other) const {
        return x < other.x;
    }
};

struct QueryInput {
    long long x0, x1, y0, y1;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;
    vector< pair<long long, long long> > pts(n);
    vector<long long> ys;
    ys.reserve(n);
    for (int i = 0; i < n; ++i) {
        cin >> pts[i].first >> pts[i].second;
        ys.push_back(pts[i].second);
    }
    int q;
    cin >> q;
    vector<QueryInput> queries(q);
    for (int i = 0; i < q; ++i) {
        cin >> queries[i].x0 >> queries[i].x1 >> queries[i].y0 >> queries[i].y1;
    }

    sort(ys.begin(), ys.end());
    ys.erase(unique(ys.begin(), ys.end()), ys.end());
    int m = (int)ys.size();

    vector< pair<long long, int> > points(n);
    for (int i = 0; i < n; ++i) {
        int yidx = (int)(lower_bound(ys.begin(), ys.end(), pts[i].second) - ys.begin()) + 1;
        points[i] = make_pair(pts[i].first, yidx);
    }
    sort(points.begin(), points.end());

    vector<Event> events;
    events.reserve(2 * q);
    vector<long long> ans(q, 0);
    for (int i = 0; i < q; ++i) {
        int l = (int)(lower_bound(ys.begin(), ys.end(), queries[i].y0) - ys.begin()) + 1;
        int r = (int)(upper_bound(ys.begin(), ys.end(), queries[i].y1) - ys.begin());
        if (l > r) continue;
        Event ev1;
        ev1.x = queries[i].x1;
        ev1.l = l;
        ev1.r = r;
        ev1.id = i;
        ev1.sign = 1;
        events.push_back(ev1);
        Event ev2;
        ev2.x = queries[i].x0 - 1;
        ev2.l = l;
        ev2.r = r;
        ev2.id = i;
        ev2.sign = -1;
        events.push_back(ev2);
    }
    sort(events.begin(), events.end());

    Fenwick bit(m);
    size_t p = 0;
    for (size_t idx = 0; idx < events.size(); ++idx) {
        const Event& ev = events[idx];
        while (p < points.size() && points[p].first <= ev.x) {
            bit.add(points[p].second, 1);
            ++p;
        }
        ans[ev.id] += ev.sign * bit.range_sum(ev.l, ev.r);
    }

    for (int i = 0; i < q; ++i) {
        cout << ans[i] << '\n';
    }
    return 0;
}