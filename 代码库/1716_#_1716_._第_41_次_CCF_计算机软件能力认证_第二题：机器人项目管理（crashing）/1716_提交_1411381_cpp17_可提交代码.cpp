#include <bits/stdc++.h>
using namespace std;

struct Task {
    int a, b;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    long long totalTime = 0;

    vector<Task> flexible;
    vector<Task> normal;

    for (int i = 0; i < n; ++i) {
        int o, t, a, b;
        cin >> o >> t >> a >> b;

        totalTime += t;

        if (o == 0) {
            flexible.push_back({a, b});
        } else {
            normal.push_back({a, b});
        }
    }

    // -----------------------------
    // 1. 灵活型：按照单位咖啡收益 b/a 降序
    // 为避免浮点数比较误差，用交叉相乘
    // -----------------------------
    sort(flexible.begin(), flexible.end(),
         [](const Task& x, const Task& y) {
             return 1LL * x.b * y.a > 1LL * y.b * x.a;
         });

    // flexibleBest[k]:
    // 最多给灵活型任务 k 杯咖啡时，最大的时间缩短量
    vector<double> flexibleBest(m + 1, 0.0);

    for (int cap = 0; cap <= m; ++cap) {
        double remain = cap;
        double gain = 0.0;

        for (const auto& task : flexible) {
            if (remain <= 0.0)
                break;

            double use = min(remain, (double)task.a);

            gain += use * (double)task.b / task.a;
            remain -= use;
        }

        flexibleBest[cap] = gain;
    }

    // -----------------------------
    // 2. 普通型：0/1 背包
    // dp[j] = 恰好使用 j 杯咖啡时的最大缩短量
    // -----------------------------
    const double NEG = -1e100;

    vector<double> dp(m + 1, NEG);
    dp[0] = 0.0;

    for (const auto& task : normal) {
        for (int j = m; j >= task.a; --j) {
            if (dp[j - task.a] > NEG / 2) {
                dp[j] = max(
                    dp[j],
                    dp[j - task.a] + task.b
                );
            }
        }
    }

    // -----------------------------
    // 3. 合并两部分
    //
    // 普通型使用 j 杯
    // 灵活型获得剩余 m-j 杯
    // -----------------------------
    double maxReduction = 0.0;

    for (int j = 0; j <= m; ++j) {
        if (dp[j] > NEG / 2) {
            maxReduction = max(
                maxReduction,
                dp[j] + flexibleBest[m - j]
            );
        }
    }

    double answer = (double)totalTime - maxReduction;

    cout << fixed << setprecision(6) << answer << '\n';

    return 0;
}