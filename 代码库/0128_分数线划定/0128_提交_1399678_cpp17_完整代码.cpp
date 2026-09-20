#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

struct Candidate {
    int id;     // 报名号
    int score;  // 笔试成绩
};

// 比较函数：成绩从高到低，成绩相同按报名号从小到大
bool cmp(const Candidate& a, const Candidate& b) {
    if (a.score != b.score) {
        return a.score > b.score;  // 成绩高的在前
    }
    return a.id < b.id;  // 成绩相同，报名号小的在前
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n, m;
    cin >> n >> m;
    
    vector<Candidate> candidates(n);
    for (int i = 0; i < n; i++) {
        cin >> candidates[i].id >> candidates[i].score;
    }
    
    // 按规则排序
    sort(candidates.begin(), candidates.end(), cmp);
    
    // 计算 m * 150% 下取整
    int threshold_pos = m * 3 / 2;  // m * 1.5 下取整
    
    // 分数线为第 threshold_pos 名的成绩（下标从0开始，所以是 threshold_pos - 1）
    int scoreLine = candidates[threshold_pos - 1].score;
    
    // 统计所有成绩 >= 分数线的人数
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (candidates[i].score >= scoreLine) {
            count++;
        }
    }
    
    // 输出分数线和进入面试的人数
    cout << scoreLine << " " << count << "\n";
    
    // 输出所有进入面试的选手
    for (int i = 0; i < count; i++) {
        cout << candidates[i].id << " " << candidates[i].score << "\n";
    }
    
    return 0;
}