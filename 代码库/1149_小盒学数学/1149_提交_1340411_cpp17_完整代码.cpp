#include <bits/stdc++.h>
using namespace std;

static const int STICK[10] = {
    6, // 0
    2, // 1
    5, // 2
    5, // 3
    4, // 4
    5, // 5
    6, // 6
    3, // 7
    7, // 8
    6  // 9
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    int maxS = n - 4; // 数字部分的火柴数（加号2根+等号2根）
    if (maxS < 0) {
        cout << 0 << "\n";
        return 0;
    }

    vector<vector<long long>> nums(maxS + 1);

    // 递归生成：从非零首位开始，避免前导0
    function<void(long long, int)> gen = [&](long long val, int cost) {
        if (cost > maxS) return;
        nums[cost].push_back(val);
        for (int d = 0; d <= 9; ++d) {
            int nc = cost + STICK[d];
            if (nc > maxS) continue;
            long long nv = val * 10 + d;
            gen(nv, nc);
        }
    };

    // 单独处理数字 0
    if (STICK[0] <= maxS) nums[STICK[0]].push_back(0);

    // 生成所有非零首位数字
    for (int first = 1; first <= 9; ++first) {
        int c = STICK[first];
        if (c > maxS) continue;
        gen(first, c);
    }

    // 排序 + 去重（理论上不会重复，但做一下更稳）
    for (int s = 0; s <= maxS; ++s) {
        auto &v = nums[s];
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());
    }

    long long ans = 0;

    // 枚举 cost(A), cost(B)，则 cost(C) 唯一确定
    for (int ca = 0; ca <= maxS; ++ca) {
        if (nums[ca].empty()) continue;
        for (int cb = 0; cb <= maxS - ca; ++cb) {
            if (nums[cb].empty()) continue;
            int cc = maxS - ca - cb;
            if (cc < 0 || cc > maxS) continue;
            if (nums[cc].empty()) continue;

            const auto &A = nums[ca];
            const auto &B = nums[cb];
            const auto &Cvec = nums[cc];

            for (long long a : A) {
                for (long long b : B) {
                    long long c = a + b;
                    if (binary_search(Cvec.begin(), Cvec.end(), c)) {
                        ++ans; // 有序计数：A,B 的顺序不同算不同式子
                    }
                }
            }
        }
    }

    cout << ans << "\n";
    return 0;
}