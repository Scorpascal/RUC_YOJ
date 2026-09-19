#include <bits/stdc++.h>
using namespace std;

/*
题目：回家 - 巴士路线规划

问题描述：
- N栋公寓在数轴上，公司在坐标S
- 巴士从S出发，员工投票决定前进方向（多数决，平票向负方向）
- 每位员工都会选择使自己回家时间最短的策略
- 求巴士送所有员工回家的总时间

核心思路：
1. 最后到达的公寓一定是第1个或第N个（最左或最右）
2. 如果 P[1] >= P[N]：
   - 第1个公寓的人更多，会先到达第1个公寓
   - 第N个公寓的人会投票帮助到达第1个公寓
   - 合并：将P[N]加到P[1]，处理子问题[1, N-1]
   - 答案 = 子问题时间 + 额外往返时间
3. 如果 P[1] < P[N]：反向处理
4. 递归直到所有公寓都在S的同一侧

关键参数opt：
- opt = 0：处理完当前子问题后会向左走
- opt = 1：处理完当前子问题后会向右走
- 如果当前要去的方向和之后的方向不一致，需要额外往返

时间复杂度：O(N)
*/

const int MAXN = 1e5 + 10;
int n, s;
int x[MAXN];           // 公寓位置
long long p[MAXN];     // 公寓住户人数

// 递归求解区间[l, r]的最优时间
// opt: 0表示处理完后会向左走，1表示会向右走
long long solve(int l, int r, int opt) {
    // 边界情况1：所有剩余公寓都在起点右侧
    if (x[l] > s) {
        // 巴士会一直向右走到x[r]
        return x[r] - s;
    }
    
    // 边界情况2：所有剩余公寓都在起点左侧
    if (x[r] < s) {
        // 巴士会一直向左走到x[l]
        return s - x[l];
    }
    
    // 一般情况：公寓分布在起点两侧
    // 比较左右两端的住户人数，决定先去哪边
    
    if (p[l] >= p[r]) {
        // 左边人更多或相等，先去左边（第l个公寓）
        // 第r个公寓的人会投票帮助先到达第l个公寓
        p[l] += p[r];  // 合并人数
        
        // 递归处理子问题[l, r-1]，处理完后会向右走(opt=1)
        long long subproblem_time = solve(l, r - 1, 1);
        
        // 如果当前处理完后要向左走(opt=0)，但子问题后要向右走
        // 需要额外从x[l]走到x[r]的时间
        long long extra_time = (opt == 0) ? (x[r] - x[l]) : 0;
        
        return subproblem_time + extra_time;
    } else {
        // 右边人更多，先去右边（第r个公寓）
        // 第l个公寓的人会投票帮助先到达第r个公寓
        p[r] += p[l];  // 合并人数
        
        // 递归处理子问题[l+1, r]，处理完后会向左走(opt=0)
        long long subproblem_time = solve(l + 1, r, 0);
        
        // 如果当前处理完后要向右走(opt=1)，但子问题后要向左走
        // 需要额外从x[r]走到x[l]的时间
        long long extra_time = (opt == 1) ? (x[r] - x[l]) : 0;
        
        return subproblem_time + extra_time;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    cin >> n >> s;
    for (int i = 1; i <= n; i++) {
        cin >> x[i] >> p[i];
    }
    
    // 初始调用：根据首尾两端人数决定初始方向
    // 如果p[1] >= p[n]，初始会向左走(opt=0)
    // 否则会向右走(opt=1)
    int initial_opt = (p[1] >= p[n]) ? 0 : 1;
    
    cout << solve(1, n, initial_opt) << '\n';
    
    return 0;
}
