#include <bits/stdc++.h>
using namespace std;

const int MAXN = 3e5 + 10;
int f[MAXN];
int a[MAXN], b[MAXN];
int n, m;

// 计算当前配置下的 f[n+m]
// x 表示前 x 个士兵设为0，后面的设为1
int calculate(int x) {
    // 初始化 f[0] = 0 (这个值在具体计算时不重要，因为我们会覆盖)
    f[0] = 0;
    
    // 设置 f[1] 到 f[n]
    for (int i = 1; i <= x; i++) {
        f[i] = 0;
    }
    for (int i = x + 1; i <= n; i++) {
        f[i] = 1;
    }
    
    // 计算 f[n+1] 到 f[n+m] (NAND门)
    for (int i = n + 1; i <= n + m; i++) {
        f[i] = !(f[a[i]] & f[b[i]]);
    }
    
    return f[n + m];
}

void solve() {
    cin >> n >> m;
    
    // 读入NAND门的连接关系
    for (int i = n + 1; i <= n + m; i++) {
        cin >> a[i] >> b[i];
    }
    
    // 计算全0时的结果
    int result0 = calculate(n);  // 全1
    // 计算全1时的结果
    int result1 = calculate(0);  // 全0
    
    // 如果两种情况结果相同，输出全0或全1都可以
    if (result0 == result1) {
        for (int i = 1; i <= n; i++) {
            cout << "0";
        }
        cout << "\n";
        return;
    }
    
    // 二分查找临界位置
    // 找到最大的 x 使得 calculate(x) == result1
    int left = 0, right = n;
    int ans = 0;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        if (calculate(mid) == result1) {
            ans = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    // 输出结果：前ans个为0，第ans+1个为x，后面为1
    for (int i = 1; i <= ans; i++) {
        cout << "0";
    }
    cout << "x";
    for (int i = ans + 2; i <= n; i++) {
        cout << "1";
    }
    cout << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    int T;
    cin >> T;
    
    while (T--) {
        solve();
    }
    
    return 0;
}
