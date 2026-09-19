#include <iostream>
#include <algorithm>
using namespace std;

const int INF = 1e9;
int n, m;
int a[10][10];
int dp[10][10];  // dp[i][j] = 到达(i,j)时的最大宝物数

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    cin >> n >> m;
    
    // 注意：题目坐标是(1,1)到(n,m)，行号从1开始
    // 但移动方向是上、左、右，意味着从下往上走
    // 即从第n行走到第1行
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            cin >> a[i][j];
        }
    }
    
    // 初始化为负无穷
    for (int i = 0; i <= n + 1; i++) {
        for (int j = 0; j <= m + 1; j++) {
            dp[i][j] = -INF;
        }
    }
    
    // 起点是(1,1)，终点是(n,m)
    // 只能往上、左、右走，说明行号只能减小或不变
    // 所以我们从第1行开始，往第n行走（这里的"上"是指行号增加）
    // 重新理解：看图，n是纵轴从下往上，m是横轴从左往右
    // (1,1)在左下角，(n,m)在右上角
    // 往上走 = 行号+1，往左走 = 列号-1，往右走 = 列号+1
    
    dp[1][1] = a[1][1];
    
    // 第一行只能往右走
    for (int j = 2; j <= m; j++) {
        dp[1][j] = dp[1][j-1] + a[1][j];
    }
    
    // 从第2行开始处理
    for (int i = 2; i <= n; i++) {
        // left[j] = 从左边走过来到达(i,j)的最大值
        // right[j] = 从右边走过来到达(i,j)的最大值
        int left[10], right[10];
        
        for (int j = 0; j <= m + 1; j++) {
            left[j] = right[j] = -INF;
        }
        
        // 先从下方上来，然后往右走
        left[1] = dp[i-1][1] + a[i][1];
        for (int j = 2; j <= m; j++) {
            left[j] = max(dp[i-1][j], left[j-1]) + a[i][j];
        }
        
        // 先从下方上来，然后往左走
        right[m] = dp[i-1][m] + a[i][m];
        for (int j = m - 1; j >= 1; j--) {
            right[j] = max(dp[i-1][j], right[j+1]) + a[i][j];
        }
        
        // 取最大值
        for (int j = 1; j <= m; j++) {
            dp[i][j] = max(left[j], right[j]);
        }
    }
    
    cout << dp[n][m] << endl;
    
    return 0;
}
