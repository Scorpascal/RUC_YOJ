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

using ll = long long;

struct Val {
    ll cost;      // 2 * 调整后的费用
    ll negCnt;    // -完成题数
};

// 字典序比较
bool operator<(const Val& a, const Val& b) {
    if (a.cost != b.cost)
        return a.cost < b.cost;
    return a.negCnt < b.negCnt;
}

Val operator+(const Val& a, const Val& b) {
    return {a.cost + b.cost, a.negCnt + b.negCnt};
}

Val operator-(const Val& a, const Val& b) {
    return {a.cost - b.cost, a.negCnt - b.negCnt};
}

// priority_queue 默认大根堆，因此反过来写
struct GreaterVal {
    bool operator()(const Val& a, const Val& b) const {
        if (a.cost != b.cost)
            return a.cost > b.cost;
        return a.negCnt > b.negCnt;
    }
};

struct Result {
    ll cnt;       // 完成题数
    ll realCost;  // 真实费用
};

int n;
ll m;

vector<ll> a, b;


/*
    reward2 = 2 * lambda

    最小化：
        2 * realCost - reward2 * cnt

    同时在调整费用相同时优先 cnt 更大。
*/
Result calc(ll reward2) {

    priority_queue<
        Val,
        vector<Val>,
        GreaterVal
    > pq;

    // 当前凸函数在 h = 0 处的函数值
    Val f0{0, 0};

    for (int i = 0; i < n; ++i) {

        // 什么都不做
        Val idle{0, 0};

        // 当天造 + 当天验
        Val both{
            2LL * (a[i] + b[i]) - reward2,
            -1
        };

        // h 不改变时的最好选择
        Val center = (both < idle ? both : idle);

        // 验一道题
        Val check{
            2LL * b[i] - reward2,
            -1
        };

        // 造一道题
        Val make{
            2LL * a[i],
            0
        };

        Val lower = center - check;
        Val upper = make - center;

        // 整个函数先整体平移
        f0 = f0 + center;

        /*
            修正最小斜率。

            凸函数的斜率必须满足新的 lower 下界。
            一天最多只需要修改一个旧斜率。
        */
        if (!pq.empty() && pq.top() < lower) {

            Val smallest = pq.top();
            pq.pop();

            f0 = f0 + (smallest - lower);

            pq.push(lower);
        }

        // 新增加的右侧斜率
        pq.push(upper);
    }

    ll cnt = -f0.negCnt;

    /*
        f0.cost =
            2 * realCost - reward2 * cnt

        所以：
            2 * realCost
          = f0.cost + reward2 * cnt
    */
    ll realCost2 =
        f0.cost + reward2 * cnt;

    return {
        cnt,
        realCost2 / 2
    };
}


int main() {

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n >> m;

    a.resize(n);
    b.resize(n);

    for (ll &x : a)
        cin >> x;

    for (ll &x : b)
        cin >> x;


    /*
        reward 越大，最优方案做的题越多。

        找一个足够大的右端点，使得最终会做满 n 题。
    */
    ll L = 0;
    ll R = 1;

    while (calc(R).cnt < n)
        R <<= 1;


    /*
        找最大的 reward2，
        使得这个 reward 下得到的最优方案
        真实费用仍然 <= m。
    */
    while (L < R) {

        ll mid = L + (R - L + 1) / 2;

        Result cur = calc(mid);

        if (cur.realCost <= m)
            L = mid;
        else
            R = mid - 1;
    }


    Result left = calc(L);

    // 已经可以全部完成
    if (left.cnt == n) {
        cout << n << '\n';
        return 0;
    }


    /*
        再看相邻的 reward2 = L + 1。
    */
    Result right = calc(L + 1);

    if (right.realCost <= m) {
        cout << right.cnt << '\n';
        return 0;
    }

    // 中间没有发生题数跳变
    if (right.cnt == left.cnt) {
        cout << left.cnt << '\n';
        return 0;
    }


    /*
        如果一次从 left.cnt 跳到了 right.cnt，
        说明这几个点位于同一段线性区间。

        每多完成一道题的边际费用相同。
    */
    ll deltaCnt =
        right.cnt - left.cnt;

    ll deltaCost =
        right.realCost - left.realCost;

    ll marginal =
        deltaCost / deltaCnt;

    ll extra =
        (m - left.realCost) / marginal;

    extra = min(extra, deltaCnt);

    cout << left.cnt + extra << '\n';

    return 0;
}