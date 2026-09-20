#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

// 定义一个结构体或pair来存储优先队列的元素
// pair<long long, int>: first 是当前机器完成任务的时间点, second 是机器处理单个任务的耗时
typedef pair<long long, int> PII;

int main() {
    // 优化输入输出效率
    ios::sync_with_stdio(false);
    cin.tie(0);

    int k, n, m;
    if (!(cin >> k >> n >> m)) return 0;

    vector<int> u(n);
    vector<int> v(m);

    // 优先队列，小顶堆，用于模拟机器调度
    // 存储 {当前累积时间, 单次加工时间}
    priority_queue<PII, vector<PII>, greater<PII>> pq1;
    priority_queue<PII, vector<PII>, greater<PII>> pq2;

    for (int i = 0; i < n; ++i) {
        cin >> u[i];
        // 初始状态：机器处理完第一个工件的时间就是它的单次耗时
        pq1.push({u[i], u[i]});
    }

    for (int i = 0; i < m; ++i) {
        cin >> v[i];
        pq2.push({v[i], v[i]});
    }

    // 存储第一阶段 k 个任务各自完成的最早时间
    vector<long long> finish1(k);
    for (int i = 0; i < k; ++i) {
        PII top = pq1.top();
        pq1.pop();
        finish1[i] = top.first;
        // 该机器处理下一个任务的时间 = 当前结束时间 + 单次耗时
        pq1.push({top.first + top.second, top.second});
    }

    // 存储第二阶段 k 个任务各自需要的处理耗时（不考虑开始时间，仅考虑持续时间）
    // 这里从小到大生成，相当于处理最快的那些坑位
    vector<long long> cost2(k);
    for (int i = 0; i < k; ++i) {
        PII top = pq2.top();
        pq2.pop();
        cost2[i] = top.first;
        pq2.push({top.first + top.second, top.second});
    }

    // 贪心匹配：
    // finish1 是从小到大的（第 i 个任务越早做完 Stage 1）
    // cost2 也是从小到大的（机器处理能力越快，或者是第 j 个“坑位”越早空闲）
    // 我们要让：Stage 1 结束得最晚的任务，去占用 Stage 2 耗时最短的坑位。
    // 即：finish1[i] + cost2[k - 1 - i] 的最大值
    long long max_time = 0;
    for (int i = 0; i < k; ++i) {
        long long total = finish1[i] + cost2[k - 1 - i];
        if (total > max_time) {
            max_time = total;
        }
    }

    cout << max_time << endl;

    return 0;
}